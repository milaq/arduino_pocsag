# Arduino POCSAG
Based on the stellar work by @on1arf

POCSAG library and demo application for Si4432-based ISM modules

It consists of:
  * A library called "Pocsag", the code to create an alpha-numeric pocsag-message

  * Two demo applications to send pocsag-messages using Si4432-based RF-modules.
    One designed for use on ISM-band frequencies (433/434, 466+, 863-870 Mhz) and a "ham" version using 430-440 Mhz.


## Hardware

RF Transceiver: `Si4432`

Board: `Arduino` (of your choice, 328P recommended)

Connection: `SPI`


## Dependencies

Copy the library folder contents (Pocsag) to your Arduino library folder.

Building also requires a modified "RadioHead" arduino library:
https://github.com/milaq/RadioHead
