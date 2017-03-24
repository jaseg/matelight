# Matelight
# Copyright (C) 2016 Sebastian Götte <code@jaseg.net>
# 
# This program is free software: you can redistribute it and/or modify
# it under the terms of the GNU General Public License as published by
# the Free Software Foundation, either version 3 of the License, or
# (at your option) any later version.
# 
# This program is distributed in the hope that it will be useful,
# but WITHOUT ANY WARRANTY; without even the implied warranty of
# MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
# GNU General Public License for more details.
# 
# You should have received a copy of the GNU General Public License
# along with this program.  If not, see <http://www.gnu.org/licenses/>.

import config
import threading
import numpy
from ctypes import *

class COLOR(Structure):
		_fields_ = [('r', c_uint8), ('g', c_uint8), ('b', c_uint8), ('a', c_uint8)]

class FRAMEBUFFER(Structure):
		_fields_ = [('data', POINTER(COLOR)), ('w', c_size_t), ('h', c_size_t)]

lib = CDLL('./libbdf.so')
lib.read_bdf_file.restype = c_void_p
lib.framebuffer_render_text.restype = POINTER(FRAMEBUFFER)
lib.framebuffer_render_text.argtypes= [c_char_p, c_void_p, c_void_p, c_size_t, c_size_t, c_size_t]

dbuf = numpy.zeros(config.frame_size*4, dtype=numpy.uint8)
printlock = threading.Lock()
def printframe(fb):
	with printlock:
		print('\0337\033[H')
		rgba = len(fb) == config.frame_size*4
		ip = numpy.frombuffer(fb, dtype=numpy.uint8)
		numpy.copyto(dbuf[0::4], ip[0::3+rgba])
		numpy.copyto(dbuf[1::4], ip[1::3+rgba])
		numpy.copyto(dbuf[2::4], ip[2::3+rgba])
		lib.console_render_buffer(dbuf.ctypes.data_as(POINTER(c_uint8)), config.display_width, config.display_height)

class Font:
	def __init__(self, fontfile='unifont.bdf'):
		self.font = lib.read_bdf_file(fontfile)
		assert self.font
		# hack to prevent unlocalized memory leak arising from ctypes/numpy/cpython interaction
		self.cbuf = create_string_buffer(config.frame_size*sizeof(COLOR))
		self.cbuflock = threading.Lock()

	def compute_text_bounds(self, text):
		textbytes = text.encode()
		textw, texth = c_size_t(0), c_size_t(0)
		res = lib.framebuffer_get_text_bounds(textbytes, self.font, byref(textw), byref(texth))
		if res:
			raise RuntimeError('Invalid text')
		return textw.value, texth.value

	def render_text(self, text, offset):
		with self.cbuflock:
			textbytes = bytes(str(text), 'UTF-8')
			res = lib.framebuffer_render_text(textbytes, self.font, self.cbuf,
					config.display_width, config.display_height, offset)
			if res:
				raise RuntimeError('Invalid text')
			return self.cbuf

unifont = Font()
