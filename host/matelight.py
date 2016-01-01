import colorsys
from itertools import product
from ctypes import c_size_t, c_uint8, c_void_p, c_float, CDLL, Structure, POINTER
import numpy as np
import time

from config import *

ml = CDLL('./libml.so')
ml.matelight_open.restype = c_void_p

if ml.matelight_usb_init():
	raise OSError('Cannot initialize USB library')
matelights = ml.matelight_open()
if matelights is None:
	raise ImportError('Cannot open any Mate Light devices')

dbuf = np.zeros(DISPLAY_WIDTH*DISPLAY_HEIGHT*4, dtype=np.uint8)
def sendframe(framedata):
	""" Send a frame to the display

	The argument contains a h * w array of 3-tuples of (r, g, b)-data or 4-tuples of (r, g, b, a)-data where the a
	channel is ignored.
	"""
	# just use the first Mate Light available
	rgba = len(framedata) == DISPLAY_WIDTH*DISPLAY_HEIGHT*4
	global dbuf
	np.copyto(dbuf[:640*(3+rgba)], np.frombuffer(framedata, dtype=np.uint8))
	ml.matelight_send_frame(matelights, dbuf.ctypes.data_as(POINTER(c_uint8)), c_size_t(CRATES_X), c_size_t(CRATES_Y), c_float(BRIGHTNESS), rgba)

