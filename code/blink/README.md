# PY32F002B blink

Bare-metal blink for a custom PY32F002BF15U6 board: red LED, then white LED, forever. Registers only, no HAL.

**Pins:** PA0 = red LED, PA1 = white LED. SWD pins (PA2, PB6) are not touched.

**Build and flash** (needs `arm-none-eabi-gcc`, `pyocd`, ST-Link V2):

    make
    make flash
