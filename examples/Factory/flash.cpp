/**
 * @file      flash.cpp
 * @author    Lewis He (lewishe@outlook.com)
 * @license   MIT
 * @copyright Copyright (c) 2024  ShenZhen XinYuan Electronic Technology Co., Ltd
 * @date      2024-05-04
 *
 */
#include "flash.h"
#include <SdFat.h>
#include <Adafruit_SPIFlash.h>
#include <Adafruit_TinyUSB.h>
#include <InternalFileSystem.h>
#include <Adafruit_LittleFS.h>

// USB Mass Storage object
Adafruit_USBD_MSC usb_msc;

#define Flash_Cs            _PINNUM(1,15)
#define Flash_Miso          _PINNUM(1,13)
#define Flash_Mosi          _PINNUM(1,12)
#define Flash_Sclk          _PINNUM(1,14)
#define Flash_WP            _PINNUM(0,7)
#define Flash_HOLD          _PINNUM(0,5)

Adafruit_FlashTransport_QSPI flashTransport(
    Flash_Sclk,
    Flash_Cs,
    Flash_Mosi,
    Flash_Miso,
    Flash_WP,
    Flash_HOLD);

Adafruit_SPIFlash flash(&flashTransport);
// file system object from SdFat
FatVolume fatfs;
File32 root;
File32 file;
const SPIFlash_Device_t flash_devs = ZD25WQ16B;


/***********************************
  ______ _                _____ _    _
 |  ____| |        /\    / ____| |  | |
 | |__  | |       /  \  | (___ | |__| |
 |  __| | |      / /\ \  \___ \|  __  |
 | |    | |____ / ____ \ ____) | |  | |
 |_|    |______/_/    \_\_____/|_|  |_|

************************************/
void listDir()
{
    if (!root.open("/")) {
        Serial.println("open root failed");
    }
    // Open next file in root.
    // Warning, openNext starts at the current directory position
    // so a rewind of the directory may be required.
    while (file.openNext(&root, O_RDONLY)) {
        file.printFileSize(&Serial);
        Serial.write(' ');
        file.printModifyDateTime(&Serial);
        Serial.write(' ');
        file.printName(&Serial);
        if (file.isDir()) {
            // Indicate a directory.
            Serial.write('/');
        }
        Serial.println();
        file.close();
    }

    if (root.getError()) {
        Serial.println("openNext failed");
    } else {
        Serial.println("Done!");
    }
}

uint32_t getFlashSize()
{
    return flash.size() / 1024;
}


// Callback invoked when received READ10 command.
// Copy disk's data to buffer (up to bufsize) and
// return number of copied bytes (must be multiple of block size)
static int32_t nRF52_msc_read_cb (uint32_t lba, void *buffer, uint32_t bufsize)
{
    // Note: SPIFLash Bock API: readBlocks/writeBlocks/syncBlocks
    // already include 4K sector caching internally. We don't need to cache it, yahhhh!!
    return flash.readBlocks(lba, (uint8_t *) buffer, bufsize / 512) ? bufsize : -1;
}

// Callback invoked when received WRITE10 command.
// Process data in buffer to disk's storage and
// return number of written bytes (must be multiple of block size)
static int32_t nRF52_msc_write_cb (uint32_t lba, uint8_t *buffer, uint32_t bufsize)
{
    // Note: SPIFLash Bock API: readBlocks/writeBlocks/syncBlocks
    // already include 4K sector caching internally. We don't need to cache it, yahhhh!!
    return flash.writeBlocks(lba, buffer, bufsize / 512) ? bufsize : -1;
}

// Callback invoked when WRITE10 command is completed (status received and accepted by host).
// used to flush any pending cache.
static void nRF52_msc_flush_cb (void)
{
    // sync with flash
    flash.syncBlocks();

    // clear file system's cache to force refresh
    fatfs.cacheClear();

}


bool setupFlash()
{
    SerialMon.print("[FLASH ] Initializing ...  ");
    if (flash.begin(&flash_devs)) {
        Serial.println("success");

        Serial.print("\tJEDEC ID: 0x");
        Serial.println(flash.getJEDECID(), HEX);
        Serial.print("\tFlash size: ");
        Serial.print(flash.size() / 1024);
        Serial.println(" KB");

        // Set disk vendor id, product id and revision with string up to 8, 16, 4 characters respectively
        usb_msc.setID("LilyGo", "External Flash", "1.0");

        // Set callback
        usb_msc.setReadWriteCallback(nRF52_msc_read_cb,
                                     nRF52_msc_write_cb,
                                     nRF52_msc_flush_cb);

        // Set disk size, block size should be 512 regardless of spi flash page size
        usb_msc.setCapacity(flash.size() / 512, 512);

        // MSC is ready for read/write
        usb_msc.setUnitReady(true);

        usb_msc.begin();

        fatfs.begin(&flash);

        listDir();

        return true;
    }
    return false;
}

void sleepFlash()
{
}

void wakeupFlash()
{
}

#define FILENAME    "/adafruit.txt"
#define CONTENTS    "Adafruit Little File System test file contents"

using namespace Adafruit_LittleFS_Namespace;

Adafruit_LittleFS_Namespace::File fileInternal(InternalFS);

bool fsCheck()
{
    // Initialize Internal File System
    InternalFS.begin();

    size_t write_size = 0;
    size_t read_size = 0;
    char buf[32] = {0};

    Adafruit_LittleFS_Namespace::File file(InternalFS);
    const char *text = "lilygo fs test";
    size_t text_length = strlen(text);
    const char *filename = "/lilygo.txt";

    Serial.println("Try create file .\n");
    if (file.open(filename, FILE_O_WRITE)) {
        write_size = file.write(text);
    } else {
        Serial.println("Open file failed .\n");
        goto FORMAT_FS;
    }

    if (write_size != text_length) {
        Serial.println("Text bytes do not match .\n");
        file.close();
        goto FORMAT_FS;
    }

    file.close();

    if (!file.open(filename, FILE_O_READ)) {
        Serial.println("Open file failed .\n");
        goto FORMAT_FS;
    }

    read_size = file.readBytes(buf, text_length);
    if (read_size != text_length) {
        Serial.println("Text bytes do not match .\n");
        file.close();
        goto FORMAT_FS;
    }

    if (memcmp(buf, text, text_length) != 0) {
        Serial.println("The written bytes do not match the read bytes .\n");
        file.close();
        goto FORMAT_FS;
    }
    return true;
FORMAT_FS:
    Serial.println("Format FS ....\n");
    InternalFS.format();
    InternalFS.begin();
    return false;
}

bool setupInternalFileSystem()
{
    // Initialize Internal File System
    InternalFS.begin();
    return fsCheck();
}





