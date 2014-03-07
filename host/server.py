#!/usr/bin/env python

from socketserver import *
from time import time, strftime, sleep
from collections import namedtuple
from itertools import product, cycle
import threading
import random

from ctypes import CDLL, POINTER, c_void_p, Structure, c_uint8, c_size_t, cast, addressof

import numpy as np

from matelight import sendframe, DISPLAY_WIDTH, DISPLAY_HEIGHT, FRAME_SIZE


UDP_TIMEOUT = 3.0


class COLOR(Structure):
		_fields_ = [('r', c_uint8), ('g', c_uint8), ('b', c_uint8), ('a', c_uint8)]

class FRAMEBUFFER(Structure):
		_fields_ = [('data', POINTER(COLOR)), ('w', c_size_t), ('h', c_size_t)]

bdf = CDLL('./libbdf.so')
bdf.read_bdf_file.restype = c_void_p
bdf.framebuffer_render_text.restype = POINTER(FRAMEBUFFER)

unifont = bdf.read_bdf_file('unifont.bdf')

def render_text(text):
	assert unifont
	fb = bdf.framebuffer_render_text(bytes(str(text), 'UTF-8'), unifont)
	fbd = fb.contents
	buf = np.ctypeslib.as_array(cast(fbd.data, POINTER(c_uint8)), shape=(fbd.h, fbd.w, 4))
	# Set data pointer to NULL before freeing framebuffer struct to prevent free_framebuffer from also freeing the data
	# buffer that is now used by numpy
	#bdf.console_render_buffer(fb)
	fbd.data = cast(c_void_p(), POINTER(COLOR))
	bdf.free_framebuffer(fb)
	return buf

printlock = threading.Lock()

def printframe(fb):
	h,w,_ = fb.shape
	printlock.acquire()
	print('\0337\033[H', end='')
	print('Rendering frame @{}'.format(time()))
	bdf.console_render_buffer(fb.ctypes.data_as(POINTER(c_uint8)), w, h)
	print('\033[0m\033[KCurrently rendering', current_entry.entrytype, 'from', current_entry.remote, ':', current_entry.text, '\0338', end='')
	printlock.release()

def scroll(text):
	""" Returns whether it could scroll all the text uninterrupted """
	log('Scrolling', text)
	fb = render_text(text);
	h,w,_ = fb.shape
	for i in range(-DISPLAY_WIDTH,w+1):
#		if current_entry.entrytype == 'udp' or (textqueue and current_entry in defaulttexts):
#			log('Aborting rendering due to new input')
#			return False
		leftpad			= np.zeros((DISPLAY_HEIGHT, max(-i, 0), 4), dtype=np.uint8)
		framedata		= fb[:, max(0, i):min(i+DISPLAY_WIDTH, w)]
		rightpad		= np.zeros((DISPLAY_HEIGHT, min(DISPLAY_WIDTH, max(0, i+DISPLAY_WIDTH-w)), 4), dtype=np.uint8)
		dice = np.concatenate((leftpad, framedata, rightpad), 1)
		sendframe(dice)
		printframe(dice)
		fb = render_text(text);
	return True

QueueEntry = namedtuple('QueueEntry', ['entrytype', 'remote', 'timestamp', 'text'])
defaultlines = [ QueueEntry('text', '127.0.0.1', 0, l[:-1].replace('\\x1B', '\x1B')) for l in open('default.lines').readlines() ]
random.shuffle(defaultlines)
defaulttexts = cycle(defaultlines)
current_entry = next(defaulttexts)
conns = {}
textqueue = []

def log(*args):
	printlock.acquire()
	print(strftime('[%m-%d %H:%M:%S]'), ' '.join(str(arg) for arg in args), '\x1B[0m')
	printlock.release()


class MateLightUDPServer(UDPServer):
    """
    The server class for the SocketServer interface.
    """
    def __init__(self, *args, **kwargs):
        """
        Setup the deque for the frame
        """
        super(MateLightUDPServer, self).__init__(*args, **kwargs)
        self.frame_deque = collections.deque(maxlen=30)
        
    def get_next_frame(self):
        try:
            return self.frame_deque.popleft()
        except IndexError:
            return None


class MateLightUDPHandler(BaseRequestHandler):
    """
    Handles one UDP connection to the matelight
    """
	def handle(self):
		try:
			# Housekeeping - FIXME: This is *so* the wrong place for this.
			for k,v in conns.items():
				if time() - v.timestamp > UDP_TIMEOUT:
					del conns[k]

			global current_entry, conns
			data = self.request[0].strip()
			if len(data) != FRAME_SIZE*3+4:
				#raise ValueError('Invalid frame size: Expected {}, got {}'.format(FRAME_SIZE+4, len(data)))
				return
			frame = data[:-4]
			#crc1, = struct.unpack('!I', data[-4:])
			#crc2, = zlib.crc32(frame, 0),
			#if crc1 != crc2:
			#	raise ValueError('Invalid frame CRC checksum: Expected {}, got {}'.format(crc2, crc1))
			#socket.sendto(b'ACK', self.client_address)
			a = np.array(list(frame), dtype=np.uint8)
			timestamp = time()
			addr = self.client_address[0]
			conn = QueueEntry('udp', addr, timestamp, '')
			if addr not in conns:
				log('New UDP connection from', addr)
				current_entry = conn
			conns[addr] = current_entry
			if current_entry.entrytype == 'udp' and current_entry.remote == addr:
				current_entry = conn
				frame = a.reshape((DISPLAY_HEIGHT, DISPLAY_WIDTH, 3))
                self.server.frame_deque.append(frame)
				#sendframe(frame)
                #printframe(np.pad(frame, ((0,0),(0,0),(0,1)), 'constant', constant_values=(0,0)))
		except Exception as e:
			log('Error receiving UDP frame:', e)

class MateLightTCPTextHandler(BaseRequestHandler):
	def handle(self):
		global current_entry, conns
		data = str(self.request.recv(1024).strip(), 'UTF-8')
		addr = self.client_address[0]
		timestamp = time()
		if len(data) > 140:
			self.request.sendall('TOO MUCH INFORMATION!\n')
			return
		log('Text from {}: {}\x1B[0m'.format(addr, data))
		textqueue.append(QueueEntry('text', addr, timestamp, data))
		self.request.sendall(b'KTHXBYE!\n')

#TCPServer.allow_reuse_address = True
#server = TCPServer(('', 1337), MateLightTCPTextHandler)
#t = threading.Thread(target=server.serve_forever)
#t.daemon = True
#t.start()

UDPServer.allow_reuse_address = True
userver = MateLightUDPServer(('', 1337), MateLightUDPHandler)
t = threading.Thread(target=userver.serve_forever)
t.daemon = True
t.start()

if __name__ == '__main__':
    while True:
        next_frame = userver.get_next_frame()
        if next_frame:
            sendframe(next_frame)
    
def bla():
	print('\033[2J'+'\n'*9)
	while True:
		if current_entry.entrytype == 'text':
			if scroll(current_entry.text):
				if current_entry in textqueue:
					textqueue.remove(current_entry)
				if textqueue:
					current_entry = textqueue[0]
				else:
					if conns:
						current_entry = random.choice(list(conns.values()))
					else:
						current_entry = next(defaulttexts)
			if current_entry.entrytype != 'udp' and textqueue:
				current_entry = textqueue[0]
		if current_entry.entrytype == 'udp':
			if time() - current_entry.timestamp > UDP_TIMEOUT:
				current_entry = next(defaulttexts)
			else:
				sleep(0.1)

