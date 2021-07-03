#include "Adafruit_EK79686.h"
#include "Adafruit_EPD.h"

#define BUSY_WAIT 500

// clang-format off

const uint8_t ek79686_default_init_code[] {
  EK79686_PSR, 1, 0x0F, // LUT from OTP 128x296
    0x4D, 1, 0xAA, // FITI cmd (???)
    0x87, 1, 0x28,
    0x84, 1, 0x00,
    0x83, 1, 0x05,
    0xA8, 1, 0xDF,
    0xA9, 1, 0x05,
    0xB1, 1, 0xE8,
    0xAB, 1, 0xA1,
    0xB9, 1, 0x10,
    0x88, 1, 0x80,
    0x90, 1, 0x02,
    0x86, 1, 0x15,
    0x91, 1, 0x8D,
    0xAA, 1, 0x0F,
    EK79686_PON, 0,
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
Adafruit_EK79686::Adafruit_EK79686(int width, int height, int8_t SID,
                                   int8_t SCLK, int8_t DC, int8_t RST,
                                   int8_t CS, int8_t SRCS, int8_t MISO,
                                   int8_t BUSY)
    : Adafruit_EPD(width, height, SID, SCLK, DC, RST, CS, SRCS, MISO, BUSY) {

  if ((width % 8) != 0) {
    width += 8 - (width % 8);
  }
  buffer1_size = ((uint32_t)width * (uint32_t)height) / 8;
  buffer2_size = buffer1_size;

  if (SRCS >= 0) {
    use_sram = true;
    buffer1_addr = 0;
    buffer2_addr = buffer1_size;
    buffer1 = buffer2 = NULL;
  } else {
    buffer1 = (uint8_t *)malloc(buffer1_size);
    buffer2 = (uint8_t *)malloc(buffer2_size);
  }
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
Adafruit_EK79686::Adafruit_EK79686(int width, int height, int8_t DC, int8_t RST,
                                   int8_t CS, int8_t SRCS, int8_t BUSY,
                                   SPIClass *spi)
    : Adafruit_EPD(width, height, DC, RST, CS, SRCS, BUSY, spi) {

  if ((height % 8) != 0) {
    height += 8 - (height % 8);
  }
  buffer1_size = (uint16_t)width * (uint16_t)height / 8;
  buffer2_size = buffer1_size;

  if (SRCS >= 0) {
    use_sram = true;
    buffer1_addr = 0;
    buffer2_addr = buffer1_size;
    buffer1 = buffer2 = NULL;
  } else {
    buffer1 = (uint8_t *)malloc(buffer1_size);
    buffer2 = (uint8_t *)malloc(buffer2_size);
  }
}

/**************************************************************************/
/*!
    @brief wait for busy signal to end
*/
/**************************************************************************/
void Adafruit_EK79686::busy_wait(void) {
  if (_busy_pin >= 0) {
    do {
      EPD_command(EK79686_FLG);
      delay(10);
    } while (!digitalRead(_busy_pin));
  } else {
    delay(BUSY_WAIT);
  }
  delay(200); // additional delay
}

/**************************************************************************/
/*!
    @brief begin communication with and set up the display.
    @param reset if true the reset pin will be toggled.
*/
/**************************************************************************/
void Adafruit_EK79686::begin(bool reset) {
  Adafruit_EPD::begin(reset);
  setBlackBuffer(0, true);  // black defaults to inverted
  setColorBuffer(1, false); // red defaults to not-inverted

  powerDown();
}

/**************************************************************************/
/*!
    @brief signal the display to update
*/
/**************************************************************************/
void Adafruit_EK79686::update() {
  EPD_command(EK79686_DRF);
  delay(10);
  busy_wait();
}

/**************************************************************************/
/*!
    @brief start up the display
*/
/**************************************************************************/
void Adafruit_EK79686::powerUp() {
  uint8_t buf[5];

  hardwareReset();
  delay(10);

  const uint8_t *init_code = ek79686_default_init_code;

  if (_epd_init_code != NULL) {
    init_code = _epd_init_code;
  }
  EPD_commandList(init_code);

  if (_epd_lut_code) {
    EPD_commandList(_epd_lut_code);
  }
  busy_wait();
}

/**************************************************************************/
/*!
    @brief wind down the display
*/
/**************************************************************************/

void Adafruit_EK79686::powerDown(void) {
  uint8_t buf[1];

  EPD_command(EK79686_POF); // power off
  busy_wait();

  buf[0] = 0xA5;
  EPD_command(EK79686_DSLP, buf, 1);
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
uint8_t Adafruit_EK79686::writeRAMCommand(uint8_t index) {
  if (index == 0) {
    return EPD_command(EK79686_DTM1, false);
  }
  if (index == 1) {
    return EPD_command(EK79686_DTM2, false);
  }
  return 0;
}

/**************************************************************************/
/*!
    @brief Some displays require setting the RAM address pointer
    @param x X address counter value
    @param y Y address counter value
*/
/**************************************************************************/
void Adafruit_EK79686::setRAMAddress(uint16_t x, uint16_t y) {}
