#!/usr/bin/env python3
# Generate an XTerm-256 color RGB palette
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

xterm_colors = [
	(0x00, 0x00, 0x00),
	(0xa8, 0x00, 0x00),
	(0x00, 0xa8, 0x00),
	(0xa8, 0x54, 0x00),
	(0x00, 0x00, 0xa8),
	(0xa8, 0x00, 0xa8),
	(0x00, 0xa8, 0xa8),
	(0xa8, 0xa8, 0xa8),
	(0x54, 0x54, 0x54),
	(0xfc, 0x54, 0x54),
	(0x54, 0xfc, 0x54),
	(0xfc, 0xfc, 0x54),
	(0x54, 0x54, 0xfc),
	(0xfc, 0x54, 0xfc),
	(0x54, 0xfc, 0xfc),
	(0xfc, 0xfc, 0xfc)]

# colors 16..232: the 6x6x6 color cube
_valuerange = (0x00, 0x5f, 0x87, 0xaf, 0xd7, 0xff)
for i in range(217):
	r = _valuerange[(i // 36) % 6]
	g = _valuerange[(i // 6) % 6]
	b = _valuerange[i % 6]
	xterm_colors.append((r, g, b))

# colors 233..253: grayscale
for i in range(1, 24):
	v = 8 + i * 10
	xterm_colors.append((v, v, v))

for r,g,b in xterm_colors:
    print("\t{{{:#04x}, {:#04x}, {:#04x}}},".format(r,g,b))
