#ifndef LIB_ADAFRUIT_UC8276
#define LIB_ADAFRUIT_UC8276

#include "Adafruit_EPD.h"
#include <Arduino.h>

#define EPD_RAM_BW 0x10
#define EPD_RAM_RED 0x13

#define UC8276_PANELSETTING 0x00
#define UC8276_POWEROFF 0x02
#define UC8276_POWERON 0x04
#define UC8276_DEEPSLEEP 0x07
#define UC8276_DISPLAYREFRESH 0x12
#define UC8276_WRITE_RAM1 0x10
#define UC8276_WRITE_RAM2 0x13
#define UC8276_WRITE_VCOM 0x50
#define UC8276_GET_STATUS 0x71

/**************************************************************************/
/*!
    @brief  Class for interfacing with UC8276 EPD drivers
*/
/**************************************************************************/
class Adafruit_UC8276 : public Adafruit_EPD {
public:
  Adafruit_UC8276(int width, int height, int8_t SID, int8_t SCLK, int8_t DC,
                  int8_t RST, int8_t CS, int8_t SRCS, int8_t MISO,
                  int8_t BUSY = -1);
  Adafruit_UC8276(int width, int height, int8_t DC, int8_t RST, int8_t CS,
                  int8_t SRCS, int8_t BUSY = -1, SPIClass *spi = &SPI);

  void begin(bool reset = true);
  void powerUp();
  void update(void);
  void powerDown();

protected:
  uint8_t writeRAMCommand(uint8_t index);
  void setRAMAddress(uint16_t x, uint16_t y);
  void setRAMWindow(uint16_t x1, uint16_t y1, uint16_t x2, uint16_t y2);
  void busy_wait();
};

#endif
