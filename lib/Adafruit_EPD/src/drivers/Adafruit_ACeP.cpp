#include "Adafruit_ACeP.h"
#include "Adafruit_EPD.h"

#define BUSY_WAIT 500

// clang-format off

const uint8_t acep_default_init_code[] {
  ACEP_PANEL_SETTING, 2, 0xEF, 0x08, // LUT from OTP
    ACEP_POWER_SETTING, 4, 0x37, 0x00, 0x23, 0x23, // 0x05&0x05?
    ACEP_POWER_OFF_SEQUENCE, 1, 0x00,
    ACEP_BOOSTER_SOFT_START, 3, 0xC7, 0xC7, 0x1D,
    ACEP_PLL, 1, 0x3C,
    ACEP_TSE, 1, 0x00,
    ACEP_CDI, 1, 0x37,
    ACEP_TCON, 1, 0x22,
    ACEP_RESOLUTION, 4, 0x02, 0x58, 0x01, 0xC0,
    ACEP_PWS, 1, 0xAA,
    0xFE};

// clang-format on

/**************************************************************************/
/*!
    @brief constructor if using external SRAM chip and software SPI
    @param width the width of the display in pixels
    @param height the height of the display in pixels
    @param SID the SID pin to use
    @param SCLK the SCLK pin to use
    @param DC the data/command pin to use
    @param RST the reset pin to use
    @param CS the chip select pin to use
    @param SRCS the SRAM chip select pin to use
    @param MISO the MISO pin to use
    @param BUSY the busy pin to use
*/
/**************************************************************************/
Adafruit_ACEP::Adafruit_ACEP(int width, int height, int8_t SID, int8_t SCLK,
                             int8_t DC, int8_t RST, int8_t CS, int8_t SRCS,
                             int8_t MISO, int8_t BUSY)
    : Adafruit_EPD(width, height, SID, SCLK, DC, RST, CS, SRCS, MISO, BUSY) {

  if ((width % 8) != 0) {
    width += 8 - (width % 8);
  }
  buffer1_size = (uint16_t)width * (uint16_t)height / 2;
  buffer2_size = 0;

  if (SRCS >= 0) {
    use_sram = true;
    buffer1_addr = 0;
    buffer2_addr = 0;
  } else {
    buffer1 = (uint8_t *)malloc(buffer1_size);
    buffer2 = NULL;
  }

  singleByteTxns = true;
}

// constructor for hardware SPI - we indicate DataCommand, ChipSelect, Reset

/**************************************************************************/
/*!
    @brief constructor if using on-chip RAM and hardware SPI
    @param width the width of the display in pixels
    @param height the height of the display in pixels
    @param DC the data/command pin to use
    @param RST the reset pin to use
    @param CS the chip select pin to use
    @param SRCS the SRAM chip select pin to use
    @param BUSY the busy pin to use
*/
/**************************************************************************/
Adafruit_ACEP::Adafruit_ACEP(int width, int height, int8_t DC, int8_t RST,
                             int8_t CS, int8_t SRCS, int8_t BUSY, SPIClass *spi)
    : Adafruit_EPD(width, height, DC, RST, CS, SRCS, BUSY, spi) {

  if ((height % 8) != 0) {
    height += 8 - (height % 8);
  }
  buffer1_size = (uint16_t)width * (uint16_t)height / 2;
  buffer2_size = 0;

  if (SRCS >= 0) {
    use_sram = true;
    buffer1_addr = 0;
    buffer2_addr = 0;
  } else {
    buffer1 = (uint8_t *)malloc(buffer1_size);
    buffer2 = buffer1;
  }

  singleByteTxns = true;
}

/**************************************************************************/
/*!
    @brief wait for busy signal to end
*/
/**************************************************************************/
void Adafruit_ACEP::busy_wait(void) {
  if (_busy_pin >= 0) {
    while (!digitalRead(_busy_pin)) { // wait for busy high
      delay(10);
    }
  } else {
    delay(BUSY_WAIT);
  }
}

/**************************************************************************/
/*!
    @brief begin communication with and set up the display.
    @param reset if true the reset pin will be toggled.
*/
/**************************************************************************/
void Adafruit_ACEP::begin(bool reset) {
  Adafruit_EPD::begin(reset);

  delay(100);
  powerDown();
}

/**************************************************************************/
/*!
    @brief signal the display to update
*/
/**************************************************************************/
void Adafruit_ACEP::update() {
  uint8_t buf[4];
  /*
  // clear data
  buf[0] = 0x02;
  buf[1] = 0x58;
  buf[2] = 0x01;
  buf[3] = 0xC0;
  EPD_command(ACEP_RESOLUTION, buf, 4);
  EPD_command(ACEP_DTM);
  for (int i=0; i< 134400/256; i++) {
    uint8_t block[256];
    memset(block, 0x77, 256);
    EPD_data(block, 256);
  }
  EPD_command(ACEP_POWER_ON);
  busy_wait();
  EPD_command(ACEP_DISPLAY_REFRESH);
  busy_wait();
  EPD_command(ACEP_POWER_OFF);

  if (_busy_pin >= 0) {
    while (digitalRead(_busy_pin)) { // wait for busy LOW
      delay(10);
    }
  } else {
    delay(BUSY_WAIT);
  }*/

  // actual data
  // clear data
  buf[0] = 0x02;
  buf[1] = 0x58;
  buf[2] = 0x01;
  buf[3] = 0xC0;
  EPD_command(ACEP_RESOLUTION, buf, 4);
  EPD_command(ACEP_DTM);
  for (int i = 0; i < 134400 / 256; i++) {
    uint8_t block[256];
    memset(block, ((i % 6) << 4) | (i % 6), 256);
    EPD_data(block, 256);
  }
  EPD_command(ACEP_POWER_ON);
  busy_wait();
  EPD_command(ACEP_DISPLAY_REFRESH);
  busy_wait();
  EPD_command(ACEP_POWER_OFF);
}

/**************************************************************************/
/*!
    @brief start up the display
*/
/**************************************************************************/
void Adafruit_ACEP::powerUp() {
  uint8_t buf[5];

  hardwareReset();
  delay(200);
  busy_wait();
  const uint8_t *init_code = acep_default_init_code;

  if (_epd_init_code != NULL) {
    init_code = _epd_init_code;
  }
  EPD_commandList(init_code);
  delay(1000);
  buf[0] = 0x37;
  EPD_command(ACEP_CDI, buf, 1);
}

/**************************************************************************/
/*!
    @brief wind down the display
*/
/**************************************************************************/

void Adafruit_ACEP::powerDown(void) {
  uint8_t buf[1];

  delay(1000);

  // deep sleep
  buf[0] = 0xA5;
  EPD_command(ACEP_DEEP_SLEEP, buf, 1);

  delay(100);
}

/**************************************************************************/
/*!
    @brief Send the specific command to start writing to EPD display RAM
    @param index The index for which buffer to write (0 or 1 or tri-color
   displays) Ignored for monochrome displays.
    @returns The byte that is read from SPI at the same time as sending the
   command
*/
/**************************************************************************/
uint8_t Adafruit_ACEP::writeRAMCommand(uint8_t index) {
  return EPD_command(ACEP_DTM, false);
}

/**************************************************************************/
/*!
    @brief Some displays require setting the RAM address pointer
    @param x X address counter value
    @param y Y address counter value
*/
/**************************************************************************/
void Adafruit_ACEP::setRAMAddress(uint16_t x, uint16_t y) {}
