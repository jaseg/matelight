#!/usr/bin/env python

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

