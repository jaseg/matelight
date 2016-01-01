#!/usr/bin/env python

import socket
from time import strftime
import itertools
import sys
from contextlib import suppress

from config import *

import matelight
import bdf
import crap


def log(*args):
	print(strftime('\x1B[93m[%m-%d %H:%M:%S]\x1B[0m'), ' '.join(str(arg) for arg in args), '\x1B[0m')
	sys.stdout.flush()

class TextRenderer:
	def __init__(self, text):
		self.text = text
		self.width, _ = unifont.compute_text_bounds(text)

	def __iter__(self):
		for i in range(-DISPLAY_WIDTH, self.width):
			yield render_text(self.text, i)

class MatelightTCPServer:
	def __init__(self, port, ip):
		self.sock = socket.socket(socket.AF_INET, socket.SOCK_STREAM)
		self.sock.setsockopt(socket.SOL_SOCKET, socket.SO_REUSEADDR, 1)
		self.sock.setblocking(blocking)
		self.sock.bind((ip, port))
		self.conns = set()
		self.renderqueue = []

	def __iter__(self):
		q, self.renderqueue = self.renderqueue, []
		for frame in itertools.chain(*q):
			yield frame

	def handle_connections(self):
		for conn in self.conns:
			try:
				line = conn.recv(1024).decode('UTF-8').strip()
				if len(data) > 140: # Unicode string length, *not* byte length of encoded UTF-8
					conn.sendall(b'TOO MUCH INFORMATION!\n')
				else:
					log('\x1B[95mText from\x1B[0m {}: {}\x1B[0m'.format(addr, data))
					renderqueue.append(TextRenderer(data))
					conn.sendall(b'KTHXBYE!\n')
			except socket.error, e:
				if err == errno.EAGAIN or err == errno.EWOULDBLOCK:
					continue
			with suppress(socket.error):
				conn.close()
			self.conns.remove(conn)

def _fallbackiter(it, fallback):
	for fel in fallback:
		for el in it:
			yield el
		yield fel

if __name__ == '__main__':
	tcp_server = MatelightTCPServer(config.tcp_addr, config.tcp_port)
	udp_server = crap.CRAPServer(config.udp_addr, config.udp_port)
	forwarder  = crap.CRAPClient(config.crap_fw_addr, config.crap_fw_port) if config.crap_fw_addr is not None else None

	def defaulttexts(filename='default.lines'):
		with open(filename) as f:
			return itertools.chain.from_iterable(( TextRenderer(l[:-1].replace('\\x1B', '\x1B')) for l in f.readlines() ))

	with suppress(KeyboardInterrupt):
		for renderer in _fallbackiter(tcp_server, defaulttexts()):
			for frame in _fallbackiter(udp_server, renderer):
				matelight.sendframe(frame)
				if forwarder:
					forwarder.sendframe(frame)

	tcp_server.close()
	udp_server.close()
	forwarder.close()
