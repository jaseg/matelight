Mate Light
==========

.. image:: http://cbag3.c-base.org/media/cache/artefact_show/uploads/assets/52f6c9c16c72d.jpeg

Transform a bunch of empty mate crates to a huge display using a 14 year old laptop, a 13US$ microcontroller board and a couple of cheap chinese christmas lights.

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
The control software is a Python script accepting framebuffer data via UDP and text via TCP. Since it is run on a 900MHz Pentium 3, the two most time-critical pieces, pixel font rendering for scrolling text and USB communication are written in C and called from the Python script via ctypes.

Architecture
~~~~~~~~~~~~
The server listens on TCP and UDP ports 1337. Any text arriving through TCP is rendered as a marquee, interpreting ANSI escape sequences (colors, blink etc.). The UDP port accepts CRAP, our Custom advanced video stReAming Protocol. A CRAP packet contains three bytes of RGB data per pixel in 40 rows of 40 columns (i.e. ``[R0,0 G0,0 B0,0 R0,1 G0,1 B0,1 ... R0,39 G0,39 B0,39 R1,0 G1,0 B1,0 ... R15,39 G15,39 B15,39]``) and an optional trailing CRC32 checksum.

There is at least one further server implementation of CRAP in the form of an `HTML5 CRAP emulator`_. There is `a Python script that plays gifs over CRAP`_.

Related Projects
----------------
* `A Python script that plays gifs over CRAP`_
* An `HTML5 CRAP emulator`_
* `A CRAP client for node.js`_
* `Webcam streaming on Mate Light`_
* `A game programming framework for Mate Light`_
* `Snake for Mate Light`_
* `Game of Life for Mate Light`_
* `Mate Light Android App`_
* `Blinkenlights for Mate Light`_
* `Postillon Newsticker for Matelight`_
* `Movie Streaming and Live 3D Raytracing for Mate-Light in Javascript`_
* `An audio spectrum analyzer`_

.. _`A Python script that plays gifs over CRAP`: https://github.com/uwekamper/matelight-gifplayer
.. _`HTML5 CRAP emulator`: https://github.com/sodoku/matelightemu
.. _`A CRAP client for node.js`: https://github.com/sodoku/node-matelight
.. _`Postillon Newsticker for Matelight`: https://gist.github.com/XenGi/9168633
.. _`Webcam streaming on Mate Light`: https://github.com/c-base/matetv
.. _`A game programming framework for Mate Light`: https://github.com/c-base/pymlgame
.. _`Snake for Mate Light`: https://github.com/c-base/pymlsnake
.. _`Game of Life for Mate Light`: https://github.com/igorw/conway-php#mate-light
.. _`Blinkenlights for Mate Light`: https://github.com/igorw/matelight-blm
.. _`Mate Light Android App`: https://github.com/cketti/MateLightAndroid
.. _`Movie Streaming and Live 3D Raytracing for Mate-Light in Javascript`: https://github.com/MichaelKreil/matelight-playground
.. _`An audio spectrum analyzer`: https://github.com/c-base/mlaudiospectrum

