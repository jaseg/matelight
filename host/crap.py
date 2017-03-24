# Matelight
# Copyright (C) 2016 Sebastian Götte <code@jaseg.net>
# Copyright (C) 2015 Uwe Kamper <me@uwekamper.de>
# 
# This program is free software: you can redistribute it and/or modify
# it under the terms of the GNU General Public License as published by
# the Free Software Foundation, either version 3 of the License, or
# (at your option) any later version.
# 
# This program is distributed in the hope that it will be useful,
# but WITHOUT ANY WARRANTY; without even the implied warranty of
# MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
# GNU General Public License for more details.
# 
# You should have received a copy of the GNU General Public License
# along with this program.  If not, see <http://www.gnu.org/licenses/>.

import socket
import struct
import zlib
import io
from time import time
import numpy

import config

class CRAPClient:
	def __init__(self, ip='127.0.0.1', port=1337):
		self.ip, self.port = ip, port
		self.sock = socket.socket(socket.AF_INET, socket.SOCK_DGRAM)
		self.close = self.sock.close

	def sendframe(self, frame):
		fb = numpy.frombuffer(frame, dtype=numpy.uint8)
		fb.shape = config.frame_size, len(frame)/config.frame_size
		self.sock.sendto(fb[:,:3].tobytes(), (self.ip, self.port))


def _timestamped_recv(sock):
	while True:
		try:
			data, addr = sock.recvfrom(config.frame_size*3+4)
		except io.BlockingIOError as e:
			raise StopIteration()
		except socket.timeout:
			raise StopIteration()
		else:
			yield time(), data, addr


class CRAPServer:
	def __init__(self, ip='', port=1337, blocking=False, log=print):
		self.sock = socket.socket(socket.AF_INET, socket.SOCK_DGRAM)
		self.sock.setsockopt(socket.SOL_SOCKET, socket.SO_REUSEADDR, 1)
		self.sock.setblocking(blocking)
		self.sock.bind((ip, port))

		self.blocking = blocking
		self.current_client = None
		self.last_timestamp = 0
		self.begin_timestamp = 0
		self.log = log

	def close(self):
		self.sock.close()

	def __iter__(self):
		for timestamp, data, (addr, sport) in _timestamped_recv(self.sock):
			if data is None:
				yield None, None

			if timestamp - self.last_timestamp >= config.udp_timeout\
					or timestamp - self.begin_timestamp > config.udp_switch_interval:
				self.current_client = addr
				self.begin_timestamp = timestamp
				self.log('\x1B[91mAccepting UDP data from\x1B[0m', addr)
				self.sock.settimeout(config.udp_timeout)

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
				yield 'udp:'+addr, data
		self.current_client = None
		self.sock.settimeout(None if self.blocking else 0)


