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

from pixelterm.pixelterm import termify_pixels

class MockImage:
    def __init__(self, nparray):
        self.nparray = nparray
        self.im = self

    @property
    def size(self):
        h, w, _ = self.nparray.shape
        return (w, h)

    def getpixel(self, pos):
        x, y = pos
        return tuple(self.nparray[y][x])

def printframe(framedata):
    print(termify_pixels(MockImage(framedata)))

