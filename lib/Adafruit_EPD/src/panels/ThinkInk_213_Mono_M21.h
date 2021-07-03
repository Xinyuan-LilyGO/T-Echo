#ifndef _THINKINK_213_MONO_M21_H
#define _THINKINK_213_MONO_M21_H

// This file is #included by Adafruit_ThinkInk.h and does not need to
// #include anything else to pick up the EPD header or ink mode enum.

class ThinkInk_213_Mono_M21 : public Adafruit_UC8151D {
public:
  ThinkInk_213_Mono_M21(int8_t SID, int8_t SCLK, int8_t DC, int8_t RST,
                        int8_t CS, int8_t SRCS, int8_t MISO, int8_t BUSY = -1)
      : Adafruit_UC8151D(212, 104, SID, SCLK, DC, RST, CS, SRCS, MISO, BUSY){};

  ThinkInk_213_Mono_M21(int8_t DC, int8_t RST, int8_t CS, int8_t SRCS,
                        int8_t BUSY = -1, SPIClass *spi = &SPI)
      : Adafruit_UC8151D(212, 104, DC, RST, CS, SRCS, BUSY, spi){};

  void begin(thinkinkmode_t mode = THINKINK_MONO) {
    Adafruit_UC8151D::begin(true);
    setColorBuffer(1, true); // layer 1 uninverted
    setBlackBuffer(1, true); // only one buffer

    inkmode = mode; // Preserve ink mode for ImageReader or others

    layer_colors[EPD_WHITE] = 0b00;
    layer_colors[EPD_BLACK] = 0b01;
    layer_colors[EPD_RED] = 0b01;
    layer_colors[EPD_GRAY] = 0b01;
    layer_colors[EPD_LIGHT] = 0b00;
    layer_colors[EPD_DARK] = 0b01;

    default_refresh_delay = 1000;
    setRotation(0);
    powerDown();
  }
};

#endif // _THINKINK_213_MONO_M21_H
