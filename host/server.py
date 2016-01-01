#!/usr/bin/env python

import socket
import time
import itertools
import sys
from contextlib import suppress
import asyncio
import threading

import config

import matelight
import bdf
import crap


def log(*args):
	print(time.strftime('\x1B[93m[%m-%d %H:%M:%S]\x1B[0m'), ' '.join(str(arg) for arg in args), '\x1B[0m')
	sys.stdout.flush()

class TextRenderer:
	def __init__(self, text, title='default', font=bdf.unifont):
		self.text = text
		self.font = font
		(self.width, _), _testrender  = font.compute_text_bounds(text), font.render_text(text, 0)
		self.title = title

	def __iter__(self):
		for i in range(-config.display_width, self.width):
			yield self.title, self.font.render_text(self.text, i)
			time.sleep(0.05)

class MatelightTCPServer:
	def __init__(self, ip, port, loop):
		coro = asyncio.start_server(self.handle_conn, ip, port, loop=loop)
		server = loop.run_until_complete(coro)
		self.renderqueue = []
		self.close = server.close

	def __iter__(self):
		q, self.renderqueue = self.renderqueue, []
		for title, frame in itertools.chain(*q):
			yield title, frame

	async def handle_conn(self, reader, writer):
		line = (await reader.read(1024)).decode('UTF-8').strip()
		if len(line) > 140: # Unicode string length, *not* byte length of encoded UTF-8
			writer.write(b'TOO MUCH INFORMATION!\n')
		else:
			addr,*rest = writer.get_extra_info('peername')
			log('\x1B[95mText from\x1B[0m {}: {}\x1B[0m'.format(addr, line))
			try:
				self.renderqueue.append(TextRenderer(line, title='tcp:'+addr))
			except:
				writer.write(b'STAHPTROLLINK?\n')
			else:
				writer.write(b'KTHXBYE!\n')
		await writer.drain()
		writer.close()

def _fallbackiter(it, fallback):
	for fel in fallback:
		for el in it:
			yield el
		yield fel

def defaulttexts(filename='default.lines'):
	with open(filename) as f:
		return itertools.chain.from_iterable(( TextRenderer(l[:-1].replace('\\x1B', '\x1B')) for l in f.readlines() ))

if __name__ == '__main__':
	try:
		ml = matelight.Matelight(config.ml_usb_serial_match)
	except ValueError as e:
		print(e, 'Starting in headless mode.', file=sys.stderr)
		ml = None

	loop = asyncio.get_event_loop()

	tcp_server = MatelightTCPServer(config.tcp_addr, config.tcp_port, loop)
	udp_server = crap.CRAPServer(config.udp_addr, config.udp_port)
	forwarder  = crap.CRAPClient(config.crap_fw_addr, config.crap_fw_port) if config.crap_fw_addr is not None else None

	async_thr = threading.Thread(target=loop.run_forever)
	async_thr.daemon = True
	async_thr.start()

	with suppress(KeyboardInterrupt):
		while True:
			for title, frame in _fallbackiter(udp_server, _fallbackiter(tcp_server, defaulttexts())):
				if ml:
					ml.sendframe(frame)
				if forwarder:
					forwarder.sendframe(frame)

	tcp_server.close()
	udp_server.close()
	forwarder.close()
