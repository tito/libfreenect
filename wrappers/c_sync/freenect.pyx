#!/usr/bin/env python
#
# Copyright (c) 2010 Brandyn White (bwhite@dappervision.com)
#                    Andrew Miller (amiller@dappervision.com)
#
# This code is licensed to you under the terms of the Apache License, version
# 2.0, or, at your option, the terms of the GNU General Public License,
# version 2.0. See the APACHE20 and GPL2 files for the text of the licenses,
# or the following URLs:
# http://www.apache.org/licenses/LICENSE-2.0
# http://www.gnu.org/licenses/gpl-2.0.txt
#
# If you redistribute this file in source form, modified or unmodified, you
# may:
#   1) Leave this header intact and distribute it under the same terms,
#      accompanying it with the APACHE20 and GPL20 files, or
#   2) Delete the Apache 2.0 clause and accompany it with the GPL2 file, or
#   3) Delete the GPL v2 clause and accompany it with the APACHE20 file
# In all cases you must keep the copyright notice intact and include a copy
# of the CONTRIB file.
#
# Binary distributions must follow the binary distribution requirements of
# either License.
import cython

DEPTH_BYTES = 614400 # 480 * 640 * 2
RGB_BYTES = 921600  # 480 * 640 * 3


cdef extern from "stdlib.h":
    void free(void *ptr)


cdef extern from "freenect_sync.h":
    char *freenect_get_depth()
    char *freenect_get_rgb()
    void freenect_sync_stop()


cdef extern from "Python.h":
    object PyString_FromStringAndSize(char *s, Py_ssize_t len)


def get_depth():
    """Get the next available depth frame from the kinect.

    Returns:
        A python string for the 16 bit depth image (640*480*2 bytes)
    """
    cdef char* depth = freenect_get_depth()
    depth_str = PyString_FromStringAndSize(depth, DEPTH_BYTES)
    free(depth);
    return depth_str


def get_rgb():
    """Get the next available depth frame from the kinect.

    Returns:
        A python string for the 16 bit depth image (640*480*2 bytes)
    """
    cdef char* rgb = freenect_get_rgb()
    rgb_str = PyString_FromStringAndSize(rgb, RGB_BYTES)
    free(rgb);
    return rgb_str


def sync_stop():
    """Terminate the synchronous runloop if running, else this is a NOP
    """
    freenect_sync_stop()