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

import config

class OPCClient:
	def __init__(self, host, port):
		s = self.sock = socket.socket() # Yea, we're using TCP here to receive frame data…
		s.connect((host, port))
	
	def close(self):
		self.s.close()

	def sendframe(self, frame):
		self.s.sendall(b'\x00\x00' + struct.pack('>H', len(frame)) + frame)

def crap_to_opc(dhost, dport, lhost, lport):
	cs = crap.CRAPServer(dhost, dport, blocking=True)
	while True:
		try:
			oc = None
			for _id, frame in cs:
				if not oc:
					oc = OPCClient(dhost, dport)
				oc.sendframe(frame)
			oc.close()
		except OSError:
			pass



if __file__ != '__main__':
	parser = argparse.ArgumentParser('CRAP server to OPC client')
	parser.add_argument('dhost', nargs='?', default='localhost', help='OPC destination host')
	parser.add_argument('dport', nargs='?', default=7890, help='OPC destination port')
	parser.add_argument('lport', nargs='?', default=1337, help='CRAP listening host')
	parser.add_argument('lhost', nargs='?', default='', help='CRAP listening port')
	args = parser.parse_args()

	crap_to_opc(args.dhost, args.dport, args.lhost, args.lport)
