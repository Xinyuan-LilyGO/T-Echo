#ifndef _THINKINK_270_TRICOLOR_Z70_H
#define _THINKINK_270_TRICOLOR_Z70_H

// This file is #included by Adafruit_ThinkInk.h and does not need to
// #include anything else to pick up the EPD header or ink mode enum.

class ThinkInk_270_Tricolor_Z70 : public Adafruit_EK79686 {
public:
  ThinkInk_270_Tricolor_Z70(int8_t SID, int8_t SCLK, int8_t DC, int8_t RST,
                            int8_t CS, int8_t SRCS, int8_t MISO,
                            int8_t BUSY = -1)
      : Adafruit_EK79686(264, 176, SID, SCLK, DC, RST, CS, SRCS, MISO, -1){};

  ThinkInk_270_Tricolor_Z70(int8_t DC, int8_t RST, int8_t CS, int8_t SRCS,
                            int8_t BUSY = -1, SPIClass *spi = &SPI)
      : Adafruit_EK79686(264, 176, DC, RST, CS, SRCS, -1, spi){};

  void begin(thinkinkmode_t mode = THINKINK_TRICOLOR) {
    Adafruit_EK79686::begin(true);

    inkmode = mode; // Preserve ink mode for ImageReader or others

    layer_colors[EPD_WHITE] = 0b10;
    layer_colors[EPD_BLACK] = 0b01;
    layer_colors[EPD_RED] = 0b10;
    layer_colors[EPD_GRAY] = 0b10;
    layer_colors[EPD_LIGHT] = 0b10;
    layer_colors[EPD_DARK] = 0b01;

    default_refresh_delay = 13000;
    powerDown();
  }
};

#endif // _THINKINK_270_TRICOLOR_Z70_H
