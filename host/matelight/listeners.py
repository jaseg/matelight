#!/usr/bin/env python3
from socketserver import *
import threading
import zlib
import struct
import host
import numpy as np
import time
import renderers
from PIL import Image, ImageSequence
from config import *
# Loading frame (for the big font file)
img = Image.open(open('../nyancat.png', 'rb'))
frame = np.array(img.convert('RGB').getdata(), dtype=np.uint8).reshape((DISPLAY_HEIGHT, DISPLAY_WIDTH, 3))
host.sendframe(frame)
from font import *

class ThreadingTCPServer(ThreadingMixIn, TCPServer): pass
class ThreadingUDPServer(ThreadingMixIn, UDPServer): pass

default_renderer = renderers.TextRenderer('\x1B[91mFeed \x1B[92mme via \x1B[93mTCP on \x1B[94;101mml.jaseg.net:1337\x1B[0;91m ! \x1B[95mI can\x1B[96m parse\x1B[97m ANSI\x1B[92m color\x1B[93m codes\x1B[91m!\x1B[38;5;207m http://github.com/jaseg/matelight')
global renderer, count
renderer = default_renderer
count = 0

class MateLightUDPHandler(BaseRequestHandler):
	def handle(self):
		data = self.request[0].strip()
		if len(data) != FRAME_SIZE: #+4
			raise ValueError('Invalid frame size: Expected {}, got {}'.format(FRAME_SIZE, len(frame))) #+4
		frame = data[:-4]
		#crc1, = struct.unpack('!I', data[-4:])
		crc2 = zlib.crc32(frame),
		if crc1 != crc2:
			raise ValueError('Invalid frame CRC checksum: Expected {}, got {}'.format(crc2, crc1))
		#socket.sendto(b'ACK', self.client_address)
		a = np.array(frame, dtype=np.uint8)
		a.reshape((DISPLAY_HEIGHT, DISPLAY_WIDTH, 3))
		host.sendframe(a)

class MateLightTCPTextHandler(BaseRequestHandler):
	def handle(self):
		try:
			data = str(self.request.recv(1024).strip(), 'UTF-8')
			if len(data) > 140:
				self.request.sendall('TOO MUCH INFORMATION!\n')
				return
			global renderer, count
			print(data+'\x1B[0m')
			renderer = renderers.TextRenderer(data)
			count = 3
			self.request.sendall(b'KTHXBYE!\n')
		except:
			pass

TCPServer.allow_reuse_address = True
server = TCPServer(('', 1337), MateLightTCPTextHandler)
t = threading.Thread(target=server.serve_forever)
t.daemon = True
t.start()

UDPServer.allow_reuse_address = True
userver = UDPServer(('', 1337), MateLightUDPHandler)
t = threading.Thread(target=userver.serve_forever)
t.daemon = True
t.start()

while True:
	global renderer, count
	foo = renderer
	if count == 0:
		renderer = default_renderer
	else:
		count = count - 1
	for frame, delay in foo.frames():
		#print(list(frame.flatten()))
		host.sendframe(np.swapaxes(frame, 0, 1))
		#time.sleep(delay)

