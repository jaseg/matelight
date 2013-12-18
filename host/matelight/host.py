from pyusb import usb
import colorsys
import numpy as np

dev = usb.core.find(idVendor=0x1cbe, idProduct=0x0003)

def sendframe(framedata):
    if not isinstance(framedata, np.array) or framedata.shape != (DISPLAY_WIDTH, DISPLAY_HEIGHT, 3) or framedata.dtype != np.int8:
        raise ValueError('framedata must be a ({}, {}, 3)-numpy array of int8s'.format(DISPLAY_WIDTH, DISPLAY_HEIGHT))

    for cx, cy in itertools.product(range(16), range(2)):
        cratedata = framedata[cx*CRATE_WIDTH:(cx+1)*CRATE_WIDTH, cy*CRATE_HEIGHT:(cy+1)*CRATE_HEIGHT]
        # Send framebuffer data
        dev.write(0x01, bytes([0, x, y])+bytes(list(cratedata.flatten())))
    # Send latch command
    dev.write(0x01, b'\x01')
