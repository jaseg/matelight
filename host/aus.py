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

from socketserver import *
import socket
import struct
import zlib
from time import time, strftime, sleep
from collections import namedtuple, deque
import itertools
import threading
import random
import os

from ctypes import *

from matelight import sendframe, DISPLAY_WIDTH, DISPLAY_HEIGHT, FRAME_SIZE

if __name__ == '__main__':
    sendframe(bytes([0, 0, 0]*FRAME_SIZE))

