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
    print('FB STRUCT: w {} h {} memory location {:x}'.format(fbd.w, fbd.h, addressof(fbd.data.contents)))
    print("TYPE:", type(fbd.data))
    casted = cast(fbd.data, POINTER(c_uint8))
    print("CASTED TYPE:" , type(casted))
    buf = np.ctypeslib.as_array(cast(fbd.data, POINTER(c_uint8)), shape=(fbd.h, fbd.w, 4))
    print('RAW DATA:')
    alen = fbd.h*fbd.w
#    for i in range(alen):
#        c = fbd.data[i]
#        print('{:x}'.format(addressof(c)), c.r, c.g, c.b)
    print('BUF', buf.shape, buf.dtype)
#    for x, y, z in product(range(fbd.h), range(fbd.w), range(4)):
#        print(buf[x][y][z])

    # Set data pointer to NULL before freeing framebuffer struct to prevent free_framebuffer from also freeing the data
    # buffer that is now used by numpy
    bdf.console_render_buffer(fb)
    fbd.data = cast(c_void_p(), POINTER(COLOR))
    bdf.free_framebuffer(fb)
    print('Rendered {} chars into a buffer of w {} h {} numpy buf {}'.format(len(text), fbd.w, fbd.h, buf.shape))
    return buf

if __name__ == '__main__':
    fb = render_text('foobarbaz');
    print('buffer shape {} dtype {}'.format(fb.shape, fb.dtype))
#   import colorsys
#    h, w, _ = fb.shape
#    for x, y in product(range(w), range(h)):
#        r,g,b = colorsys.hsv_to_rgb(x*0.03+y*0.05, 1, 1)
#        fb[x][y] = (r,g,b,0)
#    print(fb)
    sendframe(fb)
    #printframe(fb)


