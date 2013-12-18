#!/usr/bin/env python3

from renderers import TextRenderer, ImageRenderer
import host, config
import time, math

score = lambda now, last, lifetime, priority, item: priority*math.log(now-last)/(10.0+item.duration)

class FuzzyQueue:
	def __init__(self, default):
		self._default = default
		self.put(default, 0.0, 0)
		self._l = []

	def put(self, item, priority=1.0, lifetime=0):
		lifetime += time.time()
		self._l.append((0, lifetime, priority, item))

	def pop(self):
		""" Get an item from the queue

		NOTE: This is *not* a regular pop, as it does not necessarily remove the item from the queue.
		"""
		now = time.time()
		# Choose item based on last used and priority
		_, index, (_, lifetime, priority, item) = max(sorted([(score(now, *v), i, v) for i, v in self._l]))
		# If item's lifetime is exceeded, remove
		if lifetime < now and item is not self._default:
			del self._l[index]
		# Otherwise, set item's last played time
		self._l[index] = (now, lifetime, prioity, item)
		# Finally, return
		return item

q = FuzzyQueue()

def insert_text(text, priority=1.0, lifetime=0, escapes=True):
	q.put(TextRenderer(text, escapes), priority, lifetime)

def insert_image(image, priority=1.0, lifetime=0):
	q.put(ImageRenderer(image), priority, lifetime)

def render_thread():
	while True:
		start = time.time()
		for frame, delay in q.pop().frames(start):
			then = time.time()
			if then-start+delay > RENDERER_TIMEOUT:
				break
			sendframe(frame)
			now = time.time()
			time.sleep(min(RENDERER_TIMEOUT, delay - (now-then)))
