#ifndef _THINKINK_270_TRICOLOR_C44_H
#define _THINKINK_270_TRICOLOR_C44_H

// This file is #included by Adafruit_ThinkInk.h and does not need to
// #include anything else to pick up the EPD header or ink mode enum.

// clang-format off

static const uint8_t ti_270c44_tri_init_code[] {
    IL91874_BOOSTER_SOFT_START, 3, 0x07, 0x07, 0x17,
    IL91874_POWER_ON, 0,
    0xFF, 200,
    IL91874_PANEL_SETTING, 1, 0x0F, // OTP lut
    IL91874_PDRF, 1, 0x00,

    0xF8, 2, 0x60, 0xA5, // boost
    0xF8, 2, 0x73, 0x23, // boost
    0xF8, 2, 0x7C, 0x00, // boost

    0xFE // EOM
};

// clang-format on

class ThinkInk_270_Tricolor_C44 : public Adafruit_IL91874 {
public:
  ThinkInk_270_Tricolor_C44(int8_t SID, int8_t SCLK, int8_t DC, int8_t RST,
                            int8_t CS, int8_t SRCS, int8_t MISO,
                            int8_t BUSY = -1)
      : Adafruit_IL91874(264, 176, SID, SCLK, DC, RST, CS, SRCS, MISO, -1){};

  ThinkInk_270_Tricolor_C44(int8_t DC, int8_t RST, int8_t CS, int8_t SRCS,
                            int8_t BUSY = -1, SPIClass *spi = &SPI)
      : Adafruit_IL91874(264, 176, DC, RST, CS, SRCS, -1, spi){};

  void begin(thinkinkmode_t mode = THINKINK_TRICOLOR) {
    Adafruit_IL91874::begin(true);
    setBlackBuffer(0, false);
    setColorBuffer(1, false);

    _epd_init_code = ti_270c44_tri_init_code;
    _epd_lut_code = NULL;

    inkmode = mode; // Preserve ink mode for ImageReader or others

    layer_colors[EPD_WHITE] = 0b00;
    layer_colors[EPD_BLACK] = 0b01;
    layer_colors[EPD_RED] = 0b10;
    layer_colors[EPD_GRAY] = 0b10;
    layer_colors[EPD_LIGHT] = 0b00;
    layer_colors[EPD_DARK] = 0b01;

    default_refresh_delay = 13000;
    powerDown();
  }
};

#endif // _THINKINK_270_TRICOLOR_C44_H
