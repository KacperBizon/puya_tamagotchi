# PY32F002B peripheral test

Bare-metal test firmware for a custom PY32F002BF15U6 board. Registers only, no HAL.

| Pin | Function | Test |
|-----|----------|------|
| PA0 / PA1 | red / white LED (high = on) | BUTTON_1 / BUTTON_2 held |
| PA3 | LDR_DRIVE (powers the LDR) | BUTTON_4 toggles it |
| PA4-PA7 | BUTTON_1..4 | states shown on OLED |
| PB0 | TIM1_CH2 PWM, passive buzzer | BUTTON_3 held; tone steps 1/2/3/4 kHz on each release |
| PB1 | ADC_IN0, LDR output | value, percent and bar on OLED |
| PB3 / PB4 | I2C1 SCL / SDA, SH1107 128x128 OLED (0x3C) | status screen |

PA2 (SWCLK) and PB6 (SWDIO) are not touched.

Build and flash (needs `arm-none-eabi-gcc`, `pyocd`, ST-Link V2):

    make
    make flash

Code layout: `main.c` (pins, buzzer, ADC, buttons, screen layout), `i2c.c` (I2C1 driver), `oled.c` (SH1107 display), `timebase.c` (SysTick), `py32.h` (shared chip definitions).

Options: `BTN_ACTIVE_LOW` in `main.c`, `I2C_FAST` in `i2c.c`, `OLED_ADDR` / `OLED_DCDC_CMD` / `OLED_SEG_REMAP` / `OLED_COM_SCAN` in `oled.c`. All of them can also be set with `-D` flags in the Makefile.
