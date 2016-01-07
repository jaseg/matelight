#!/usr/bin/env python3

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
