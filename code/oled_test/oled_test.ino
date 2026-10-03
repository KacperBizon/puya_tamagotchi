#include <Arduino.h>
#include <U8g2lib.h>
#include <Wire.h>

// Initialize 1.5" SH1107 128x128 with Page Buffer mode
U8G2_SH1107_SEEED_128X128_1_HW_I2C u8g2(U8G2_R0, /* reset=*/U8X8_PIN_NONE);

void setup(void) { u8g2.begin(); }

void loop(void) {
  u8g2.firstPage();
  do {
    // 1. Draw a Box (X, Y, Width, Height)
    u8g2.drawFrame(0, 0, 128, 128); // Outline the entire screen

    // 2. Draw a Line (X1, Y1, X2, Y2)
    u8g2.drawLine(0, 20, 127, 20); // Horizontal line under the title

    // 3. Draw Text
    u8g2.setFont(u8g2_font_ncenB08_tr); // Set a font
    // Note: Text Y-coordinate is the BOTTOM-left of the first character
    u8g2.drawStr(15, 14, "OLED Graphics");

    // 4. Draw Shapes
    u8g2.drawCircle(32, 64, 15);  // Empty circle (X_center, Y_center, radius)
    u8g2.drawDisc(96, 64, 15);    // Filled circle
    u8g2.drawBox(44, 90, 40, 20); // Filled rectangle (X, Y, W, H)

  } while (u8g2.nextPage());

  delay(1000);
}
