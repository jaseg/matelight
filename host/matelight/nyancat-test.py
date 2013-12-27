#!/usr/bin/env python3
import host
import numpy as np
from config import *
from PIL import Image, ImageSequence
import time

img1 = Image.open(open('../nyancat.png', 'rb'))
img2 = Image.open(open('../nyancat2.png', 'rb'))
scroller = Image.open(open('../scroller.png', 'rb'))
datas = []
for img in [img1, img2]:
	im = img.convert("RGB")
	im.thumbnail((DISPLAY_WIDTH, DISPLAY_HEIGHT), Image.NEAREST)
	data = np.array(im.getdata(), dtype=np.uint8)
	datas += [data.reshape((DISPLAY_HEIGHT, DISPLAY_WIDTH, 3))]

im = scroller.convert("RGB")
bar = np.array(im.getdata(), dtype=np.uint8)
foo = bar.reshape((DISPLAY_HEIGHT, 300, 3))

while True:
	for i in range(20):
		for data in datas:
			host.sendframe(data)
			time.sleep(0.1)
	for i in range(260):
		host.sendframe(foo[:, i:i+40, :])

