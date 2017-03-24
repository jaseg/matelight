#!/usr/bin/env python3
# This script eats CRAP and outputs a live GIF-over-HTTP stream. The required bandwidth is about 20kB/s
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

import io
import math
import threading
import struct
import argparse
from itertools import chain
from contextlib import suppress

from PIL import Image, ImageFile
from flask import Flask, Response

import config

import crap


frame = None
frame_cond = threading.Condition()

def framestream():
	global frame
	while True:
		with frame_cond:
			frame_cond.wait()
			img = Image.frombuffer('RGB', (config.display_width, config.display_height), frame, 'raw', 'RGB', 0, 1)
			yield encode_image(img)

def putframe(f):
	global frame
	with frame_cond:
		frame = f
		frame_cond.notify_all()

def file_headers(w, h):
	yield (b'GIF89a'
		+ struct.pack('<H', w) # image width
		+ struct.pack('<H', h) # image height
		+ b'\x70' # flags bits 2-4: color resolution = 7
		+ b'\x00' # background color index
		+ b'\x00' # pixel aspect ratio (1:1)
		# Netscape headers
		+ b'\x21' # extension block
		+ b'\xff' # app extension
		+ b'\x0b' # block size
		+ b'NETSCAPE2.0' # app name
		+ b'\x03' # sub-block size
		+ b'\x01' # loop sub-block id
		+ b'\x01\x00' # repeat count (0 == Infinity)
		+ b'\x00' # block terminator
		)

def encode_image(img):
	img = img.convert('P', palette=Image.ADAPTIVE, colors=256)
	palbytes = img.palette.palette
#	assert len(img.palette) in (2, 4, 8, 0x10, 0x20, 0x40, 0x80, 0x100)
	imio = io.BytesIO()
	ImageFile._save(img, imio, [("gif", (0, 0)+img.size, 0, 'P')])
	return (
		# GCE
		  b'\x21' # extension block
		+ b'\xf9' # graphic control extension
		+ b'\x04' # block size
		+ b'\x00' # flags
		+ b'\x00\x00' # frame delay
		+ b'\x00' # transparent color index
		+ b'\x00' # block terminator
		# Image headers
		+ b'\x2c' # image block
		+ b'\x00\x00' # image x
		+ b'\x00\x00' # image y
		+ struct.pack('<H', img.width) # image width
		+ struct.pack('<H', img.height) # image height
		+ bytes((0x80 | int(math.log(len(palbytes)//3, 2.0))-1,)) # flags (local palette and palette size)
		# Palette
		+ palbytes
		# LZW code size
		+ b'\x08'
		# Image data. We're using pillow here because I was too lazy finding a suitable LZW encoder.
		+ imio.getbuffer()
		# End of image data
		+ b'\x00'
		)


app = Flask(__name__)
#app.debug = True

@app.route('/')
def hello_world():
	return Response(chain(file_headers(config.display_width, config.display_height), framestream()), mimetype='image/gif')


if __name__ == '__main__':
	parser = argparse.ArgumentParser()
	parser.add_argument('addr', default='127.0.0.1', nargs='?')
	parser.add_argument('port', type=int, default=1337, nargs='?')
	args = parser.parse_args()

	udp_server = crap.CRAPServer(args.addr, args.port, blocking=True, log=lambda *_a: None)

	def receiver():
		for _title, frame in udp_server:
			putframe(frame)
	frame_runner = threading.Thread(target=receiver, daemon=True)
	frame_runner.start()

	app.run()
	udp_server.close()
