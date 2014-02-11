Mate Light
==========
Transform a bunch of empty mate crates to a huge display using a 10 year old laptop, a 13US$ microcontroller board and a couple of cheap chinese christmas lights.

Hardware Setup
--------------
Each bottle contains a LED with a WS2801 driver. Each crate contains a chain of 20 LEDs with a 9 pin SUB-D connector. A wiring harness connects 8 crates in series to a TI Stellaris Launchpad. The Launchpad has four hardware SPI interfaces which can control one wiring harness each. The controller board is connected to a ThinkPad T22 running the control program via USB.

1. Take a mate bottle and wrap it in aluminum foil
2. Poke a 12mm hole in the lid
3. Put it in a crate
4. Repeat from step 1
5. Stick a loop of LEDs through the lids of the bottles
6. Repeat from step 1
7. Stack up a bunch of these crates and tie them together with zip ties and straps
8. Connect the wiring harness to the LED loops and the control board
9. ???
10. PROFIT!!

Software
--------
Currently, there are two versions of the control software around. The terribly slow legacy Python script and the WIP C implementation. The C implementation is about 10.000 times faster than the python script which is why we are working on it.

Architecture
~~~~~~~~~~~~
The server listens on TCP and UDP ports 1337. Any GIF data coming in over TCP is displayed, text is first rendered interpreting ANSI escape sequences (colors, blink etc.) and rendered as a marquee. The UDP port accepts CRAP, our Custom advanced video stReAming Protocol. A CRAP packet contains three bytes of RGB data per pixel in 40 rows of 40 columns (i.e. ``[R0,0 G0,0 B0,0 R0,1 G0,1 B0,1 ... R0,39 G0,39 B0,39 R1,0 G1,0 B1,0 ... R15,39 G15,39 B15,39]``).

There is at least one further server implementation of CRAP in the form of an `HTML5 CRAP emulator`_. There is `a Python script that plays gifs over CRAP`_.

Related Projects
----------------
 * `A Python script that plays gifs over CRAP`_
 * `An HTML5 CRAP emulator`_
 * `Webcam streaming on Mate Light`_
 * `A game programming framework for Mate Light`_
 * `Snake for Mate Light`_
 * `Game of Life for Mate Light`_

.. _`A Python script that plays gifs over CRAP`: https://github.com/uwekamper/matelight-gifplayer
.. _`An HTML5 CRAP emulator`: https://github.com/sodoku/matelightemu/blob/master/app.js
.. _`Webcam streaming on Mate Light`: https://github.com/c-base/matetv
.. _`A game programming framework for Mate Light`: https://github.com/c-base/pymlgame
.. _`Snake for Mate Light`: https://github.com/c-base/pymlsnake
.. _`Game of Life for Mate Light`: https://github.com/igorw/conway-php#mate-light

