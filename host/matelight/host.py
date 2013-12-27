import usb
import colorsys
import numpy as np
from config import *
import itertools

dev = usb.core.find(idVendor=0x1cbe, idProduct=0x0003)

def sendframe(framedata):
	# not isinstance(framedata, np.array) or 
	if framedata.shape != (DISPLAY_HEIGHT, DISPLAY_WIDTH, 3) or framedata.dtype != np.uint8:
		raise ValueError('framedata must be a ({}, {}, 3)-numpy array of int8s. Got a {}-numpy array of {}'.format(DISPLAY_WIDTH, DISPLAY_HEIGHT, framedata.shape, framedata.dtype))

	for cy, cx in itertools.product(range(CRATES_Y), range(CRATES_X)):
		cratedata = framedata[cy*CRATE_HEIGHT:(cy+1)*CRATE_HEIGHT, cx*CRATE_WIDTH:(cx+1)*CRATE_WIDTH]
		# Send framebuffer data
		dev.write(0x01, bytes([0, cx, cy])+bytes(list(cratedata.flatten())))
	# Send latch command
	dev.write(0x01, b'\x01')
