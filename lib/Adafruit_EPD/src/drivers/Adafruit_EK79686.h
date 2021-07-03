#ifndef LIB_ADAFRUIT_EK79686
#define LIB_ADAFRUIT_EK79686

#include "Adafruit_EPD.h"
#include <Arduino.h>

#define EK79686_PSR 0x00
#define EK79686_PWR 0x01
#define EK79686_POF 0x02
#define EK79686_PFS 0x03
#define EK79686_PON 0x04
#define EK79686_PMEAS 0x05
#define EK79686_BTST 0x06
#define EK79686_DSLP 0x07
#define EK79686_DTM1 0x10
#define EK79686_DSP 0x11
#define EK79686_DRF 0x12
#define EK79686_DTM2 0x13
#define EK79686_PDTM1 0x14
#define EK79686_PDTM2 0x15
#define EK79686_PDRF 0x16
#define EK79686_LUT1 0x20
#define EK79686_LUTWW 0x21
#define EK79686_LUTBW 0x22
#define EK79686_LUTWB 0x23
#define EK79686_LUTBB 0x24
#define EK79686_LUTC 0x25
#define EK79686_SETVCOM 0x26
#define EK79686_OSC 0x30
#define EK79686_TSC 0x40
#define EK79686_TSE 0x41
#define EK79686_TSW 0x42
#define EK79686_TSR 0x43
#define EK79686_CDI 0x50
#define EK79686_LPD 0x51
#define EK79686_TCON 0x60
#define EK79686_TRES 0x61
#define EK79686_GSST 0x62
#define EK79686_REV 0x70
#define EK79686_FLG 0x71
#define EK79686_AMV 0x80
#define EK79686_VV 0x81
#define EK79686_VDCS 0x82
#define EK79686_PGM 0xA0
#define EK79686_APG 0xA1
#define EK79686_ROTP 0xA2
#define EK79686_CCSET 0xE0
#define EK79686_TSSET 0xE5
#define EK79686_LVD 0xE6
#define EK79686_PNLBRK 0xE7
#define EK79686_PWRSAV 0xE8
#define EK79686_AUTOSEQ 0xE9

/**************************************************************************/
/*!
    @brief  Class for interfacing with EK79686 EPD drivers
*/
/**************************************************************************/
class Adafruit_EK79686 : public Adafruit_EPD {
public:
  Adafruit_EK79686(int width, int height, int8_t SID, int8_t SCLK, int8_t DC,
                   int8_t RST, int8_t CS, int8_t SRCS, int8_t MISO,
                   int8_t BUSY = -1);
  Adafruit_EK79686(int width, int height, int8_t DC, int8_t RST, int8_t CS,
                   int8_t SRCS, int8_t BUSY = -1, SPIClass *spi = &SPI);

  void begin(bool reset = true);
  void powerUp();
  void powerDown();
  void update();

protected:
  uint8_t writeRAMCommand(uint8_t index);
  void setRAMAddress(uint16_t x, uint16_t y);
  void busy_wait();
};

#endif
