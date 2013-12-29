#!/usr/bin/env python3
import host
import numpy as np
from config import *
import time
import renderers

while True:
	renderer = renderers.TextRenderer('\x1B[91mThe \x1B[92;105mquick\x1B[0m brown \x1B[96;5mfox jumps over\x1B[0m the lazy dog.')
	for frame, delay in renderer.frames():
		#print(list(frame.flatten()))
		host.sendframe(np.swapaxes(frame, 0, 1))
		#time.sleep(delay)

