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
	for cx, cy in product(range(CRATES_X), range(CRATES_Y)):
		data = [ v for x in range(CRATE_WIDTH) for y in range(CRATE_HEIGHT) for v in framedata[cy*CRATE_HEIGHT + y][cx*CRATE_WIDTH + x][:3] ]
#		data = [0,0,0]*CRATE_WIDTH*CRATE_HEIGHT
#		for x, y in product(range(CRATE_WIDTH), range(CRATE_HEIGHT)):
#			r, g, b, _ = framedata[cy*CRATE_HEIGHT + y][cx*CRATE_WIDTH + x]
#			data[x*3 + y*CRATE_WIDTH*3 + 0] = r
#			data[x*3 + y*CRATE_WIDTH*3 + 1] = g
#			data[x*3 + y*CRATE_WIDTH*3 + 2] = b
#			print(r, g, b)
		#data = [ framedata[DISPLAY_WIDTH*3*(cy*CRATE_HEIGHT + y) + 3*(cx*CRATE_WIDTH + x)+c] for x in range(CRATE_WIDTH) for y in range(CRATE_HEIGHT) for c in range(3) ]
		if len(data) != FRAME_SIZE:
			raise ValueError('Invalid frame data. Expected {} bytes, got {}.'.format(FRAME_SIZE, len(data)))
		# Send framebuffer data
		dev.write(0x01, bytes([0, cx, cy])+bytes(data))
	# Send latch command
	dev.write(0x01, b'\x01')

