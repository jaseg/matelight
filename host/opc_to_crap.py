#!/usr/bin/env python3
# Matelight
# Copyright (C) 2016 Sebastian Götte <code@jaseg.net>
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

import struct
import socket
import argparse

import crap

class OPCServer:
	def __init__(self, host, port):
		s = self.sock = socket.socket() # Yea, we're using TCP here to receive frame data…
		s.bind((host, port))
		s.listen(0)
	
	def __iter__(self):
		conn, _addr = self.sock.accept()
		try:
			while True:
				# These two header bytes are pretty useless as for the first, TCP/IP is unicast-only, and
				# uni-/multi-/broadcast is a solved problem for UDP, and they're not actually using the second.
				if conn.recv(2, socket.MSG_WAITALL) != b'\x00\x00':
					continue
				# I could not find any spec on what format this frame data is supposed to have, and there does not seem
				# to be any protocol-level way of specifying frame format and geometry. So, we're just passing this
				# stuff through here.
				# BTW, we're not going to check the frame length here either.
				yield conn.recv(struct.unpack('>H', conn.recv(2, socket.MSG_WAITALL)), socket.MSG_WAITALL)
		except:
			raise StopIteration()
		finally:
			conn.close()

def opc_to_crap(dhost, dport, lhost, lport):
	while True:
		cc = crap.CRAPClient(dhost, dport)
		for frame in OPCServer(lhost, lport):
			cc.sendframe(frame)


if __file__ != '__main__':
	parser = argparse.ArgumentParser('OPC server to CRAP client')
	parser.add_argument('dhost', nargs='?', default='localhost', help='CRAP destination host')
	parser.add_argument('dport', nargs='?', default=1337, help='CRAP destination port')
	parser.add_argument('lport', nargs='?', default=7890, help='OPC listening host')
	parser.add_argument('lhost', nargs='?', default='', help='OPC listening port')
	args = parser.parse_args()

	opc_to_crap(args.dhost, args.dport, args.lhost, args.lport)
