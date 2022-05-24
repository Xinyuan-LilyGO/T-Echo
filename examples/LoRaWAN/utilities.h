#pragma once

#include <Arduino.h>

#ifndef _PINNUM
#define _PINNUM(port, pin)    ((port)*32 + (pin))
#endif

#define EPD_MISO            _PINNUM(1, 6)
#define EPD_MOSI            _PINNUM(0,29)
#define EPD_SCLK            _PINNUM(0,31)
#define EPD_CS              _PINNUM(0,30)
#define EPD_DC              _PINNUM(0,28)
#define EPD_RST             _PINNUM(0, 2)
#define EPD_BUSY            _PINNUM(0, 3)
#define EPD_BACKLIGHT       _PINNUM(1,11)

#define LORA_MISO           _PINNUM(0,23)
#define LORA_MOSI           _PINNUM(0,22)
#define LORA_SCLK           _PINNUM(0,19)
#define LORA_CS             _PINNUM(0,24)
#define LORA_RST            _PINNUM(0,25)
#define LORA_DIO0           _PINNUM(0,15)
#define LORA_DIO1           _PINNUM(0,20)
#define LORA_DIO2           _PINNUM(0,17)
#define LORA_DIO3           _PINNUM(0,21)
#define LORA_BUSY           _PINNUM(0,17)

#define FLASH_CS            _PINNUM(1,15)
#define FLASH_SCLK          _PINNUM(1,14)
#define FLASH_MISO          _PINNUM(1,13)
#define FLASH_MOSI          _PINNUM(1,12)

#define TOUCH_PIN           _PINNUM(0,11)
#define ADC_PIN             _PINNUM(0, 4)

#define SDA_PIN             _PINNUM(0,26)
#define SCL_PIN             _PINNUM(0,27)

#define RTC_INT_PIN         _PINNUM(0,16)

#define GPS_RX_PIN          _PINNUM(1, 9)
#define GPS_TX_PIN          _PINNUM(1, 8)
#define GPS_WAKEUP_PIN      _PINNUM(1, 2)
#define GPS_RESET_PIN       _PINNUM(1, 5)
#define GPS_PPS_PIN         _PINNUM(1, 4)

#define BUTTON_PIN          _PINNUM(1,10)

#define POWER_EN_PIN        _PINNUM(0,12)

#define GREEN_LED_PIN       _PINNUM(1, 1)
#define RED_LED_PIN         _PINNUM(1, 3)
#define BLUE_LED_PIN        _PINNUM(0,14)
