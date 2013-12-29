#!/usr/bin/env python3
from socketserver import *
import socket
import threading
import zlib
import random
import struct
import host
import numpy as np
import time
import sys
import traceback
import renderers
from PIL import Image, ImageSequence
from config import *
# Loading frame (for the big font file)
img = Image.open(open('../nyancat.png', 'rb'))
frame = np.array(img.convert('RGB').getdata(), dtype=np.uint8).reshape((DISPLAY_HEIGHT, DISPLAY_WIDTH, 3))
host.sendframe(frame)
from font import *

UDP_THRES = 1.0

class ThreadingTCPServer(ThreadingMixIn, TCPServer): pass
class ThreadingUDPServer(ThreadingMixIn, UDPServer): pass

default_renderers = [renderers.TextRenderer('\x1B[92mMate Light\x1B[93m@\x1B[92mPlay store or \x1B[94;101mtcp://ml.jaseg.net:1337\x1B[0;91m ♥ '),
	renderers.TextRenderer('\x1B[92mMate Light\x1B[0;91m ♥ \x1B[92mUnicode'),
	renderers.TextRenderer('\x1B[92mMate Light\x1B[0m powered by \x1B[95mMicrosoft™ \x1B[96mMarquee Manager® Pro')]
global renderer, count
renderer = default_renderers[0]
count = 0
lastudp = 0

class MateLightUDPHandler(BaseRequestHandler):
	def handle(self):
		try:
			global lastudp
			data = self.request[0].strip()
			if len(data) != FRAME_SIZE+4:
				#raise ValueError('Invalid frame size: Expected {}, got {}'.format(FRAME_SIZE+4, len(data)))
				return
			frame = data[:-4]
			crc1, = struct.unpack('!I', data[-4:])
			crc2, = zlib.crc32(frame, 0),
			#if crc1 != crc2:
			#	raise ValueError('Invalid frame CRC checksum: Expected {}, got {}'.format(crc2, crc1))
			#socket.sendto(b'ACK', self.client_address)
			a = np.array(list(frame), dtype=np.uint8)
			lastudp = time.time()
			host.sendframe(a.reshape((DISPLAY_HEIGHT, DISPLAY_WIDTH, 3)))
		except Exception as e:
			print('Error receiving UDP frame:', e)
			ex_type, ex, tb = sys.exc_info()
			traceback.print_tb(tb)

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
	global renderer, count, lastudp
	foo = renderer
	if count == 0:
		renderer = random.choice(default_renderers)
	else:
		count = count - 1
	for frame, delay in foo.frames():
		#print(list(frame.flatten()))
		now = time.time()
		if now-lastudp > UDP_THRES:
			host.sendframe(np.swapaxes(frame, 0, 1))
		else:
			time.sleep(0.1)

