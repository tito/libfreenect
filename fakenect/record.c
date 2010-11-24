#include <sys/stat.h>
#include <sys/types.h>
#include <libfreenect.h>
#include <stdio.h>
#include <signal.h>
#include <string.h>
#include <stdlib.h>
#include <sys/time.h>

char *out_dir;
volatile sig_atomic_t running = 1;
uint32_t last_timestamp = 0;
FILE *index_fp = NULL;


double get_time() {
    struct timeval cur;
    gettimeofday(&cur, NULL);
    return cur.tv_sec + cur.tv_usec / 1000000.;
}

void dump_depth(FILE *fp, void *data, int data_size) {
    fprintf(fp, "P5 %d %d 65535\n", FREENECT_FRAME_W, FREENECT_FRAME_H);
    fwrite(data, data_size, 1, fp);
}

void dump_rgb(FILE *fp, void *data, int data_size) {
    fprintf(fp, "P6 %d %d 255\n", FREENECT_FRAME_W, FREENECT_FRAME_H);
    fwrite(data, data_size, 1, fp);
}

FILE *open_dump(char type, double cur_time, uint32_t timestamp, int data_size, const char *extension) {
    char *fn = malloc(strlen(out_dir) + 50);
    sprintf(fn, "%c-%f-%u-%u.%s", type, cur_time, timestamp, data_size, extension);
    fprintf(index_fp, "%s\n", fn);
    sprintf(fn, "%s/%c-%f-%u-%u.%s", out_dir, type, cur_time, timestamp, data_size, extension);
    FILE* fp = fopen(fn, "w");
    if (!fp) {
	printf("Error: Cannot open file [%s]\n", fn);
	exit(1);
    }
    printf("%s\n", fn);
    free(fn);
    return fp;
}

void dump(char type, uint32_t timestamp, void *data, int data_size) {
    // timestamp can be at most 10 characters, we have a few extra
    double cur_time = get_time();
    last_timestamp = timestamp;
    switch (type) {
    case 'd':
	{
	    FILE *fp = open_dump(type, cur_time, timestamp, data_size, "pgm");
	    dump_depth(fp, data, data_size);
	    fclose(fp);
	}
	break;
    case 'r':
	{
	    FILE *fp = open_dump(type, cur_time, timestamp, data_size, "ppm");
	    dump_rgb(fp, data, data_size);	    
	    fclose(fp);
	}
	break;
    case 'a':
	{
	    FILE *fp = open_dump(type, cur_time, timestamp, data_size, "dump");
	    fwrite(data, data_size, 1, fp);
	    fclose(fp);
	}
	break;
    }
}

void snapshot_accel(freenect_device *dev) {
    if (!last_timestamp)
	return;
    int data_size = (sizeof(int16_t) + sizeof(double)) * 3;
    int16_t *i = malloc(data_size);
    double *d = (double *)(i + 3);
    freenect_get_raw_accel(dev, i, i + 1, i + 2);
    freenect_get_mks_accel(dev, d, d + 1, d + 2);
    // NOTE: We just use the last timestamp that we were given
    dump('a', last_timestamp, i, data_size);
    free(i);
}


void depth_cb(freenect_device *dev, void *depth, uint32_t timestamp) {
    dump('d', timestamp, depth, FREENECT_DEPTH_SIZE);
}


void rgb_cb(freenect_device *dev, freenect_pixel *rgb, uint32_t timestamp) {
    dump('r', timestamp, rgb, FREENECT_RGB_SIZE);
}

void init() {
    freenect_context *ctx;
    freenect_device *dev;
    if (freenect_init(&ctx, 0)) {
	printf("Error: Cannot get context\n");
	return;
    }
	
    if (freenect_open_device(ctx, &dev, 0)) {
	printf("Error: Cannot get device\n");
	return;
    }
    freenect_set_depth_format(dev, 0);
    freenect_start_depth(dev);
    freenect_set_rgb_format(dev, FREENECT_FORMAT_RGB);
    freenect_start_rgb(dev);
    freenect_set_depth_callback(dev, depth_cb);
    freenect_set_rgb_callback(dev, rgb_cb);
    while(running && freenect_process_events(ctx) >= 0)
	snapshot_accel(dev);
    freenect_stop_depth(dev);
    freenect_stop_rgb(dev);
    freenect_close_device(dev);
    freenect_shutdown(ctx);
}

void signal_cleanup (int num) {
    running = 0;
    printf("Caught signal, cleaning up\n");
    signal(SIGINT, signal_cleanup);
}

int main(int argc, char **argv) {
    if (argc != 2) {
	printf("Usage: ./record <out_dir>\n");
	return 1;
    }
    out_dir = argv[1];
    mkdir(out_dir, S_IRWXU | S_IRWXG | S_IRWXO);
    signal(SIGINT, signal_cleanup);
    char *fn = malloc(strlen(out_dir) + 50);
    sprintf(fn, "%s/INDEX.txt", out_dir);
    index_fp = fopen(fn, "r");
    if (index_fp) {
	printf("Error: Index already exists, to avoid overwriting use a different directory.\n");
	return 1;
    }
    index_fp = fopen(fn, "w");
    if (!index_fp) {
	printf("Error: Cannot open file [%s]\n", fn);
	return 1;
    }
    free(fn);
    init();
    fclose(index_fp);
    return 0;
}