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

def sendframe(framedata, index):
	""" Send a frame to the display

	The argument contains a h * w array of 3-tuples of (r, g, b)-data or 4-tuples of (r, g, b, a)-data where the a
	channel is ignored.
	"""
	for y in range(DISPLAY_HEIGHT):
		for x in range(DISPLAY_WIDTH):
			r, g, b, _ = framedata[y][x]
			print('#' if r > 10 else ' ', end='')
		print()
	for cx, cy in product(range(CRATES_X), range(CRATES_Y)):
		data = [0,0,0]*CRATE_WIDTH*CRATE_HEIGHT
		print('CRATE X {} Y {}'.format(cx, cy))
		for y in range(CRATE_HEIGHT):
			for x in range(CRATE_WIDTH):
				r, g, b, _ = framedata[cy*CRATE_HEIGHT + y][cx*CRATE_WIDTH + x]
#				import colorsys
#				r,g,b = colorsys.hsv_to_rgb(cx*0.1+cy*0.2, 1, 1)
#				r,g,b = int(r*255), int(g*255), int(b*255)
				#r,g,b = (255,0,255) if cy*CRATES_X+cx == index else (0,0,0)
				#print('#' if r > 10 else ' ', end='')
				data[x*3 + y*CRATE_WIDTH*3 + 0] = r
				data[x*3 + y*CRATE_WIDTH*3 + 1] = g
				data[x*3 + y*CRATE_WIDTH*3 + 2] = b
			print()
		#data = [ v for x in range(CRATE_WIDTH) for y in range(CRATE_HEIGHT) for v in framedata[cy*CRATE_HEIGHT + y][cx*CRATE_WIDTH + x][:3] ]
		#data = [ framedata[DISPLAY_WIDTH*3*(cy*CRATE_HEIGHT + y) + 3*(cx*CRATE_WIDTH + x)+c] for x in range(CRATE_WIDTH) for y in range(CRATE_HEIGHT) for c in range(3) ]
		data = framedata[cy*CRATE_HEIGHT:(cy+1)*CRATE_HEIGHT, cx*CRATE_WIDTH:(cx+1)*CRATE_WIDTH, :3].flat
		if len(data) != FRAME_SIZE:
			raise ValueError('Invalid frame data. Expected {} bytes, got {}.'.format(FRAME_SIZE, len(data)))
		# Send framebuffer data
		dev.write(0x01, bytes([0, cx, cy])+bytes(data))
	# Send latch command
	dev.write(0x01, b'\x01')

