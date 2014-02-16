import usb
import colorsys
import numpy as np
import itertools

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
	def chunks(l, n):
		for i in xrange(0, len(l), n):
			yield l[i:i+n]

	for cx, cy in itertools.product(range(DISPLAY_WIDTH), range(DISPLAY_HEIGHT)):
		data = [ v for x in range(CRATE_WIDTH) for y in range(CRATE_HEIGHT) for v in framedata[cy*CRATE_HEIGHT + y][cx*CRATE_WIDTH + x][:3] ]
		if len(data) != FRAME_SIZE:
			raise ValueError('Invalid frame data. Expected {} bytes, got {}.'.format(FRAME_SIZE, len(data)))
		# Send framebuffer data
		dev.write(0x01, bytes([0, x, y])+bytes(data))
	# Send latch command
	dev.write(0x01, b'\x01')

