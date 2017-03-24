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

import colorsys
from itertools import product
from ctypes import c_size_t, c_uint8, c_void_p, c_float, CDLL, Structure, POINTER
import numpy as np
import time
import atexit

import config

ml = CDLL('./libml.so')
ml.matelight_open.restype = c_void_p

if ml.matelight_usb_init():
	raise OSError('Cannot initialize USB library')

atexit.register(ml.matelight_usb_destroy)

class Matelight:
	def __init__(self, match_serial=None):
		""" Open the matelight matching the USB serial number given as a bytes object. If match_serial is None, open the
		first matelight """
		self.handle = ml.matelight_open(match_serial)
		self.dbuf = np.zeros(config.frame_size*4, dtype=np.uint8)
		if self.handle is None:
			raise ValueError('Cannot find requested matelight.')

	def sendframe(self, framedata):
		""" Send a frame to the display

		The argument contains a h * w array of 3-tuples of (r, g, b)-data or 4-tuples of (r, g, b, a)-data where the a
		channel is ignored.
		"""
		# just use the first Mate Light available
		rgba = len(framedata) == config.frame_size*4
		np.copyto(self.dbuf[:640*(3+rgba)], np.frombuffer(framedata, dtype=np.uint8))
		ml.matelight_send_frame(self.handle, self.dbuf.ctypes.data_as(POINTER(c_uint8)), c_size_t(config.crates_x),
				c_size_t(config.crates_y), c_float(config.brightness), rgba)

