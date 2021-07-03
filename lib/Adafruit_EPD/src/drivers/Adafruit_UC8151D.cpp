#include "Adafruit_UC8151D.h"
#include "Adafruit_EPD.h"

#define BUSY_WAIT 500

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
Adafruit_UC8151D::Adafruit_UC8151D(int width, int height, int8_t SID,
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
Adafruit_UC8151D::Adafruit_UC8151D(int width, int height, int8_t DC, int8_t RST,
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
void Adafruit_UC8151D::busy_wait(void) {
  if (_busy_pin >= 0) {
    do {
      EPD_command(UC8151D_FLG);
      delay(10);
    } while (!digitalRead(_busy_pin));
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
void Adafruit_UC8151D::begin(bool reset) {
  Adafruit_EPD::begin(reset);
  setBlackBuffer(1, true); // black defaults to inverted
  setColorBuffer(0, true); // red defaults to inverted

  powerDown();
}

/**************************************************************************/
/*!
    @brief signal the display to update
*/
/**************************************************************************/
void Adafruit_UC8151D::update() {
  EPD_command(UC8151D_DRF);
  delay(100);
  busy_wait();
}

/**************************************************************************/
/*!
    @brief start up the display
*/
/**************************************************************************/
void Adafruit_UC8151D::powerUp() {
  uint8_t buf[5];

  // Demo code resets 3 times!
  hardwareReset();
  delay(10);
  hardwareReset();
  delay(10);
  hardwareReset();
  delay(10);

  const uint8_t *init_code = uc8151d_monofull_init_code;

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

void Adafruit_UC8151D::powerDown(void) {
  uint8_t buf[1];

  buf[0] = 0xF7;
  EPD_command(UC8151D_CDI, buf, 1);

  EPD_command(UC8151D_POF); // power off
  busy_wait();

  buf[0] = 0xA5;
  EPD_command(UC8151D_DSLP, buf, 1);
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
uint8_t Adafruit_UC8151D::writeRAMCommand(uint8_t index) {
  if (index == 0) {
    return EPD_command(UC8151D_DTM1, false);
  }
  if (index == 1) {
    return EPD_command(UC8151D_DTM2, false);
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
void Adafruit_UC8151D::setRAMAddress(uint16_t x, uint16_t y) {}

/**************************************************************************/
/*!
    @brief Transfer the data stored in the buffer(s) to the display
*/
/**************************************************************************/
void Adafruit_UC8151D::displayPartial(uint16_t x1, uint16_t y1, uint16_t x2,
                                      uint16_t y2) {
  uint8_t buf[7];
  uint8_t c;

  // check rotation, move window around if necessary
  switch (getRotation()) {
  case 0:
    EPD_swap(x1, y1);
    EPD_swap(x2, y2);
    y1 = WIDTH - y1 - 1;
    y2 = WIDTH - y2 - 1;
    break;
  case 1:
    break;
  case 2:
    EPD_swap(x1, y1);
    EPD_swap(x2, y2);
    x1 = HEIGHT - x1 - 1;
    x2 = HEIGHT - x2 - 1;
    break;
  case 3:
    y1 = WIDTH - y1 - 1;
    y2 = WIDTH - y2 - 1;
    x1 = HEIGHT - x1 - 1;
    x2 = HEIGHT - x2 - 1;
  }
  if (x1 > x2)
    EPD_swap(x1, x2);
  if (y1 > y2)
    EPD_swap(y1, y2);

  /*
  Serial.print("x: ");
  Serial.print(x1);
  Serial.print(" -> ");
  Serial.println(x2);
  Serial.print("y: ");
  Serial.print(y1);
  Serial.print(" -> ");
  Serial.println(y2);
  */

  // backup & change init to the partial code
  const uint8_t *init_code_backup = _epd_init_code;
  const uint8_t *lut_code_backup = _epd_lut_code;
  _epd_init_code = _epd_partial_init_code;
  _epd_lut_code = _epd_partial_lut_code;

#ifdef EPD_DEBUG
  Serial.println("  Powering Up Partial");
  Serial.print("Partials since last full update: ");
  Serial.println(partialsSinceLastFullUpdate);
#endif

  powerUp();

  // This command makes the display enter partial mode
  EPD_command(UC8151D_PTIN);

  buf[0] = x1;
  buf[1] = x2;
  buf[2] = y1 >> 8;
  buf[3] = y1 & 0xFF;
  buf[4] = (y2) >> 8;
  buf[5] = (y2)&0xFF;
  buf[6] = 0x28;

  EPD_command(UC8151D_PTL, buf, 7); // resolution setting

  // buffer 1 has the old data from the last update
  if (use_sram) {
    if (partialsSinceLastFullUpdate == 0) {
      // first partial update
      sram.erase(buffer1_addr, buffer1_size, 0xFF);
    }
    writeSRAMFramebufferToEPD(buffer1_addr, buffer1_size, 0, true);
  } else {
    if (partialsSinceLastFullUpdate == 0) {
      // first partial update
      memset(buffer1, 0xFF, buffer1_size);
    }

    writeRAMFramebufferToEPD(buffer1, buffer1_size, 0, true);
  }

  delay(2);

  // buffer 2 has the new data, that we're updating
  if (use_sram) {
    writeSRAMFramebufferToEPD(buffer2_addr, buffer2_size, 1, true);
  } else {
    writeRAMFramebufferToEPD(buffer2, buffer2_size, 1, true);
  }

#ifdef EPD_DEBUG
  Serial.println("  Update");
#endif
  update();

  // Serial.println("Partial, saving old data to secondary buffer");
  if (use_sram) {
    uint32_t remaining = buffer1_size;
    uint32_t offset = 0;
    uint8_t mcp_buf[16];
    while (remaining) {
      uint8_t to_xfer = min(sizeof(mcp_buf), remaining);

      sram.read(buffer2_addr + offset, mcp_buf, to_xfer);
      sram.write(buffer1_addr + offset, mcp_buf, to_xfer);
      offset += to_xfer;
      remaining -= to_xfer;
    }
  } else {
    memcpy(buffer1, buffer2, buffer1_size); // buffer1 has the backup
  }

  partialsSinceLastFullUpdate++;

  // change init back
  _epd_lut_code = lut_code_backup;
  _epd_init_code = init_code_backup;
}
