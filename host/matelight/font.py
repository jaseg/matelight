from bdflib import reader as bdfreader # Used to read the bitmap font

FONT = bdfreader.read_bdf(iter(open('fonts/unifont-6.3.20131020.bdf').readlines()))
FONT_HEIGHT = 16

