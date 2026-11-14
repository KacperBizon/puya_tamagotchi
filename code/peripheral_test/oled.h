/*
 * SH1107 driver 128x128 OLED over I2C.
 * The display is treated as OLED_ROWS text rows of OLED_COLS characters
 * (5x7 font, 6 px pitch, 8 px per row). No framebuffer is kept in RAM.
 * All functions return 1 on success, 0 on failure (display not answering).
 */
#ifndef OLED_H
#define OLED_H

#include <stdint.h>

#define OLED_ROWS     16u
#define OLED_COLS     21u
#define OLED_BAR_MAX  125u

int oled_init(void);                            /* needs i2c_init() first; clears the screen */
int oled_text(unsigned row, const char *s);     /* row 0..15, redraws only if changed */
int oled_bar(unsigned row, uint8_t fill);       /* fill 0..OLED_BAR_MAX, redraws only if changed */

#endif
