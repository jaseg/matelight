#!/usr/bin/env python

from ctypes import CDLL, POINTER, c_void_p, Structure, c_uint8, c_size_t, cast
import numpy as np
from matelight import sendframe

class COLOR(Structure):
	_fields_ = ('r', c_uint8), ('g', c_uint8), ('b', c_uint8), ('a', c_uint8)

class FRAMEBUFFER(Structure):
	_fields_ = ('data', POINTER(COLOR)), ('w', c_size_t), ('h', c_size_t)

bdf = CDLL('./libbdf.so')
bdf.read_bdf_file.restype = c_void_p
bdf.framebuffer_render_text.restype = POINTER(FRAMEBUFFER)

unifont = bdf.read_bdf_file('unifont.bdf')

def render_text(text):
	assert unifont
	fb = bdf.framebuffer_render_text(str(text), unifont)
	fbd = fb.contents
	buf = np.ctypeslib.as_array(cast(fbd.data, POINTER(c_uint8)), shape=(fbd.h, fbd.w, 4))
	# Set data pointer to NULL before freeing framebuffer struct to prevent free_framebuffer from also freeing the data
	# buffer that is now used by numpy
	fb.data = cast(c_void_p(), POINTER(COLOR))
	bdf.free_framebuffer(fb)
	return buf

if __name__ == '__main__':
	sendframe(render_text('test'));

