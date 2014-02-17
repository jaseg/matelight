import usb
import colorsys
import numpy as np
from itertools import product

CRATE_WIDTH = 5
CRATE_HEIGHT = 4
CRATES_X = 8
CRATES_Y = 4

DISPLAY_WIDTH = CRATES_X*CRATE_WIDTH
DISPLAY_HEIGHT = CRATES_Y*CRATE_HEIGHT
FRAME_SIZE = CRATE_WIDTH*CRATE_HEIGHT*3

dev = usb.core.find(idVendor=0x1cbe, idProduct=0x0003)

def sendframe(framedata):
	""" Send a frame to the display

	The argument contains a h * w array of 3-tuples of (r, g, b)-data or 4-tuples of (r, g, b, a)-data where the a
	channel is ignored.
	"""
	# Gamma correction
	framedata = (((framedata/255) ** 2.5) * 255).astype(np.uint8)
	for cx, cy in product(range(CRATES_X), range(CRATES_Y)):
		datar = framedata[cy*CRATE_HEIGHT:(cy+1)*CRATE_HEIGHT, cx*CRATE_WIDTH:(cx+1)*CRATE_WIDTH, :3]
		data = datar.flat
		if len(data) != FRAME_SIZE:
			raise ValueError('Invalid frame data. Expected {} bytes, got {}.'.format(FRAME_SIZE, len(data)))
		# Send framebuffer data
		dev.write(0x01, bytes([0, cx, cy])+bytes(data))
	# Send latch command
	dev.write(0x01, b'\x01')

