#!/usr/bin/env python

from ctypes import CDLL, POINTER, c_void_p, Structure, c_uint8, c_size_t, cast, addressof
import numpy as np
from itertools import product
from matelight import sendframe
from terminal import printframe

class COLOR(Structure):
        _fields_ = [('r', c_uint8), ('g', c_uint8), ('b', c_uint8), ('a', c_uint8)]

class FRAMEBUFFER(Structure):
        _fields_ = [('data', POINTER(COLOR)), ('w', c_size_t), ('h', c_size_t)]

bdf = CDLL('./libbdf.so')
bdf.read_bdf_file.restype = c_void_p
bdf.framebuffer_render_text.restype = POINTER(FRAMEBUFFER)

unifont = bdf.read_bdf_file('unifont.bdf')

def render_text(text):
    assert unifont
    fb = bdf.framebuffer_render_text(bytes(str(text), 'UTF-8'), unifont)
    fbd = fb.contents
    buf = np.ctypeslib.as_array(cast(fbd.data, POINTER(c_uint8)), shape=(fbd.h, fbd.w, 4))
    # Set data pointer to NULL before freeing framebuffer struct to prevent free_framebuffer from also freeing the data
    # buffer that is now used by numpy
    bdf.console_render_buffer(fb)
    fbd.data = cast(c_void_p(), POINTER(COLOR))
    bdf.free_framebuffer(fb)
    return buf

def printframe(fb):
    h,w,_ = fb.shape
    bdf.console_render_buffer(fb.ctypes.data_as(ctypes.POINTER(ctypes.c_uint8)), w, h)

if __name__ == '__main__':
    fb = render_text('foobarbaz');
    sendframe(fb)
#    printframe(fb)


