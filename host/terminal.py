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

