import colorsys
import numpy as np
from itertools import product
from ctypes import c_size_t, c_uint8, c_void_p, c_float, CDLL, Structure, POINTER
import time

CRATE_WIDTH = 5
CRATE_HEIGHT = 4
CRATES_X = 8
CRATES_Y = 4

DISPLAY_WIDTH = CRATES_X*CRATE_WIDTH
DISPLAY_HEIGHT = CRATES_Y*CRATE_HEIGHT
CRATE_SIZE = CRATE_WIDTH*CRATE_HEIGHT*3
FRAME_SIZE = DISPLAY_WIDTH*DISPLAY_HEIGHT

# Gamma factor
GAMMA = 2.5

# Brightness of the LEDs in percent. 1.0 means 100%.
BRIGHTNESS = 1.0

ml = CDLL('./libml.so')
ml.matelight_open.restype = c_void_p

if ml.matelight_usb_init():
	raise OSError('Cannot initialize USB library')
matelights = ml.matelight_open()
if matelights is None:
	raise ImportError('Cannot open any Mate Light devices')

def sendframe(framedata):
	""" Send a frame to the display

	The argument contains a h * w array of 3-tuples of (r, g, b)-data or 4-tuples of (r, g, b, a)-data where the a
	channel is ignored.
	"""
	# just use the first Mate Light available
	w,h,c = framedata.shape
	buf = framedata.ctypes.data_as(POINTER(c_uint8))
	ml.matelight_send_frame(matelights, buf, c_size_t(CRATES_X), c_size_t(CRATES_Y), c_float(BRIGHTNESS), c == 4)

if __name__ == '__main__':
	#foo = np.array([[(0, 0, 0, 0)]*DISPLAY_WIDTH]*DISPLAY_HEIGHT)
	#bar = np.array([[(255, 0, 255, 0)]*DISPLAY_WIDTH]*DISPLAY_HEIGHT)
	x,y = 0,0
	while True:
		x += 1
		if x == DISPLAY_WIDTH:
			x = 0
			y += 1
			if y == DISPLAY_HEIGHT:
				y = 0
		foo = np.array([[(64, 0, 0, 255)]*DISPLAY_WIDTH]*DISPLAY_HEIGHT, dtype=np.uint8)
		foo[y,x,:] = (0,64,0,255)
		#from terminal import printframe
		sendframe(foo)
		#printframe(foo)
		#sendframe(bar)
		time.sleep(0.1)
