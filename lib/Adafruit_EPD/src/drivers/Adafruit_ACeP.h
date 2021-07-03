#ifndef LIB_ADAFRUIT_ACEP
#define LIB_ADAFRUIT_ACEP

#include "Adafruit_EPD.h"
#include <Arduino.h>

#define ACEP_PANEL_SETTING 0x00
#define ACEP_POWER_SETTING 0x01
#define ACEP_POWER_OFF 0x02
#define ACEP_POWER_OFF_SEQUENCE 0x03
#define ACEP_POWER_ON 0x04
#define ACEP_BOOSTER_SOFT_START 0x06
#define ACEP_DEEP_SLEEP 0x07
#define ACEP_DTM 0x10
#define ACEP_DISPLAY_REFRESH 0x12
#define ACEP_PLL 0x30
#define ACEP_TSE 0x41
#define ACEP_CDI 0x50
#define ACEP_TCON 0x60
#define ACEP_RESOLUTION 0x61
#define ACEP_PWS 0xE3

/**************************************************************************/
/*!
    @brief  Class for interfacing with ACEP EPD drivers
*/
/**************************************************************************/
class Adafruit_ACEP : public Adafruit_EPD {
public:
  Adafruit_ACEP(int width, int height, int8_t SID, int8_t SCLK, int8_t DC,
                int8_t RST, int8_t CS, int8_t SRCS, int8_t MISO,
                int8_t BUSY = -1);
  Adafruit_ACEP(int width, int height, int8_t DC, int8_t RST, int8_t CS,
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
