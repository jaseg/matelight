
class SpamError(ValueError):
    def __str__(self):
        return ' '.join(self.args)

def check_spam(addr, data):
	pass
