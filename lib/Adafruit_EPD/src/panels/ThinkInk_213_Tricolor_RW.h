#ifndef _THINKINK_213_TRICOLOR_RW_H
#define _THINKINK_213_TRICOLOR_RW_H

// This file is #included by Adafruit_ThinkInk.h and does not need to
// #include anything else to pick up the EPD header or ink mode enum.

class ThinkInk_213_Tricolor_RW : public Adafruit_SSD1680 {
public:
  ThinkInk_213_Tricolor_RW(int8_t SID, int8_t SCLK, int8_t DC, int8_t RST,
                           int8_t CS, int8_t SRCS, int8_t MISO,
                           int8_t BUSY = -1)
      : Adafruit_SSD1680(250, 122, SID, SCLK, DC, RST, CS, SRCS, MISO, BUSY){};

  ThinkInk_213_Tricolor_RW(int8_t DC, int8_t RST, int8_t CS, int8_t SRCS,
                           int8_t BUSY = -1, SPIClass *spi = &SPI)
      : Adafruit_SSD1680(250, 122, DC, RST, CS, SRCS, BUSY, spi){};

  void begin(thinkinkmode_t mode = THINKINK_TRICOLOR) {
    Adafruit_SSD1680::begin(true);
    setBlackBuffer(0, true);
    setColorBuffer(1, false);

    inkmode = mode; // Preserve ink mode for ImageReader or others

    layer_colors[EPD_WHITE] = 0b00;
    layer_colors[EPD_BLACK] = 0b01;
    layer_colors[EPD_RED] = 0b10;
    layer_colors[EPD_GRAY] = 0b10;
    layer_colors[EPD_LIGHT] = 0b00;
    layer_colors[EPD_DARK] = 0b01;

    default_refresh_delay = 13000;
    setRotation(0);
    powerDown();
  }
};

#endif // _THINKINK_213_TRI
