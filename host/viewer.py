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

import argparse
import atexit
from contextlib import suppress

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
		while True:
			for _title, frame in udp_server:
				bdf.printframe(frame)

	udp_server.close()
