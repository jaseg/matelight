
import socket
import struct
import zlib
import io
from time import time

import config

class CRAPClient:
	def __init__(self, ip='127.0.0.1', port=1337):
		self.ip, self.port = ip, port
		self.sock = socket.Socket(socket.AF_INET, socket.SOCK_DGRAM)
		self.close = self.sock.close

	def sendframe(self, frame):
		self.sock.sendto(frame, (self.ip, self.port))


def _timestamped_recv(sock):
	while True:
		try:
			data, addr = sock.recvfrom(config.frame_size*3+4)
		except io.BlockingIOError as e:
			raise StopIteration()
		else:
			yield time(), data, addr


class CRAPServer:
	def __init__(self, ip='', port=1337, blocking=False, log=print):
		self.sock = socket.socket(socket.AF_INET, socket.SOCK_DGRAM)
		self.sock.setsockopt(socket.SOL_SOCKET, socket.SO_REUSEADDR, 1)
		self.sock.setblocking(blocking)
		self.sock.bind((ip, port))

		self.current_client = None
		self.last_timestamp = 0
		self.begin_timestamp = 0
		self.log = log

	def close(self):
		self.sock.close()

	def __iter__(self):
		for timestamp, data, (addr, sport) in _timestamped_recv(self.sock):
			if data is None:
				yield None

			if timestamp - self.last_timestamp > config.udp_timeout\
					or timestamp - self.begin_timestamp > config.udp_switch_interval:
				self.current_client = addr
				self.begin_timestamp = timestamp
				self.log('\x1B[91mAccepting UDP data from\x1B[0m', addr)

			if addr == self.current_client:
				if len(data) == config.frame_size*3+4:
					(crc1,), crc2 = struct.unpack('!I', data[-4:]), zlib.crc32(data, 0),
					data = data[:-4] # crop CRC
					if crc1 and crc1 != crc2: # crc1 zero-check for backward-compatibility
						self.log('Error receiving UDP frame: Invalid frame CRC checksum: Expected {}, got {}'.format(crc2, crc1))
						continue
				elif len(data) != config.frame_size*3:
					self.log('Error receiving UDP frame: Invalid frame size: {}'.format(len(data)))
				self.last_timestamp = timestamp
				yield data


