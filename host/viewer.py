#!/usr/bin/env python

import argparse
import atexit

import bdf
import crap

atexit.register(print, '\033[?1049l') # Restore normal screen buffer at exit

if __name__ == '__main__':
	parser = argparse.ArgumentParser()
	parser.add_argument('addr', default='127.0.0.1', nargs='?')
	parser.add_argument('port', type=int, default=1337, nargs='?')
	args = parser.parse_args()

	print('\033[?1049h'+'\n'*9)
	udp_server = crap.CRAPServer(args.addr, args.port, blocking=True, log=lambda *_a: None)

	with suppress(KeyboardInterrupt):
		for _title, frame in udp_server:
			bdf.printframe(frame)

	udp_server.close()
