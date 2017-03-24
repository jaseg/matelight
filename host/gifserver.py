#!/usr/bin/env python
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

import argparse, time, argparse, textwrap, threading, io
from urllib.parse import urlparse
from PIL import Image, GifImagePlugin, ImageSequence, ImageOps
from flask import Flask, request

import crap

import config


gif = None
gif_cond = threading.Condition()

def putgif(filelike):
	global gif
	with gif_cond:
		gif = filelike
		gif_cond.notify()


def sendgifs(target):
	thost, tport = target.split(':')
	crapthing = crap.CRAPClient(thost, int(tport))
	last_gif = None

	while True:
		with gif_cond:
			gif_cond.wait_for(lambda: last_gif != gif)

		last_gif = gif
		img = Image.open(last_gif)
		palette = img.getpalette()
		last_frame = Image.new("RGBA", img.size)
		sleepiness = min(img.info.get('duration', 100) / 1000.0, 10.0)

		for frame in ImageSequence.Iterator(img):
			#This works around a known bug in Pillow
			#See also: http://stackoverflow.com/questions/4904940/python-converting-gif-frames-to-png
			frame.putpalette(palette)
			c = frame.convert("RGBA")

			if 'background' in img.info and 'transparency' in img.info:
				last_frame.paste(c, c)
			else:
				last_frame = c 

			im = last_frame.copy()
			im = ImageOps.fit(im, (config.display_width, config.display_height), Image.NEAREST, centering=(0.5, 0.5))
			crapthing.sendframe(im.tobytes())
			time.sleep(sleepiness)
			if last_gif != gif:
				break


app = Flask(__name__)
#app.debug = True

@app.route('/', methods=('POST', 'GET'))
def index():
	if request.method == 'GET':
		return textwrap.dedent('''
			<form action="#" method="POST" enctype="multipart/form-data">
				<input name="gif" type="file" accept="image/gif">
				<input type="submit" value="upload">
			</form>''')
	else: #POST
		putgif(io.BytesIO(request.files['gif'].read()))
		return 'Done, thanks!'

if __name__ == '__main__':
	parser = argparse.ArgumentParser(description='Present a web interface to stream .gif files via CRAP to a matelight')
	parser.add_argument('target', type=str, default='localhost:1337', nargs='?', help='Target matelight, host:port notation')
	args = parser.parse_args()

	gif_sender = threading.Thread(target=sendgifs, args=(args.target,), daemon=True)
	gif_sender.start()
	app.run()

