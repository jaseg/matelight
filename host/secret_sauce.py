
from collections import defaultdict
import signal


RATELIMIT_THRESHOLD = 20
GRAYLIST_LENGTH = 8

class SpamError(ValueError):
    def __str__(self):
        return ' '.join(self.args)


def loadlist(fn):
	try:
		with open(fn) as f:
			return [ l for l in ( l.strip() for l in f.readlines() if not l.startswith('#')) if l ]
	except:
		return []

greenlist = set()

graylist = []
def do_graylist(msg):
    global graylist
    graylist = [msg] + graylist[:GRAYLIST_LENGTH-1]

blacklist = loadlist('blacklist')
badwords = loadlist('badwords')
def signal_handler(_signum, _frame):
    global blacklist, badwords
    blacklist = load_blacklist()
    badwords = load_badwords()
signal.signal(signal.SIGHUP, signal_handler)

ratelimitdict = defaultdict(lambda: 0)
def empty_ratelimit(_signum, _frame):
	global ratelimitdict
	ratelimitdict = defaultdict(lambda: 0)
signal.signal(signal.SIGALRM, empty_ratelimit)
signal.setitimer(signal.ITIMER_REAL, 60)

def check_spam(addr, data):
	ratelimitdict[addr] += 1
	try:
		if ratelimitdict[addr] > RATELIMIT_THRESHOLD:
			raise SpamError('rate-limit')
		if any(word in data.lower() for word in badwords):
			raise SpamError('badwords filter')
		if addr not in greenlist and data in graylist:
			raise SpamError('graylist')
	except SpamError as err:
		do_graylist(data)
		blacklist.append(addr)
		raise err
	greenlist.add(addr)
