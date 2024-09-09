/*
 * @Description: None
 * @version: V1.0.0
 * @Author: None
 * @Date: 2023-08-16 14:24:03
 * @LastEditors: Please set LastEditors
 * @LastEditTime: 2024-09-02 13:51:11
 * @License: GPL 3.0
 */
#pragma once

#define _PINNUM(port, pin) ((port) * 32 + (pin))

// GD25LQ128DSIGRSIGR SPI
#define GD25LQ128DSIGR_CS _PINNUM(1, 15)
#define GD25LQ128DSIGR_SCLK _PINNUM(1, 14)
#define GD25LQ128DSIGR_MOSI _PINNUM(1, 12)
#define GD25LQ128DSIGR_MISO _PINNUM(1, 13)
#define GD25LQ128DSIGR_IO0 _PINNUM(1, 12)
#define GD25LQ128DSIGR_IO1 _PINNUM(1, 13)
#define GD25LQ128DSIGR_IO2 _PINNUM(0, 7)
#define GD25LQ128DSIGR_IO3 _PINNUM(0, 5)

// LED
#define LED_1 _PINNUM(1, 1)  // Green
#define LED_2 _PINNUM(1, 3)  // Red
#define LED_3 _PINNUM(0, 14) // Blue

// GDEH0154D67-FL
#define SCREEN_WIDTH 200
#define SCREEN_HEIGHT 200
#define SCREEN_BUSY _PINNUM(0, 3)
#define SCREEN_RST _PINNUM(0, 2)
#define SCREEN_DC _PINNUM(0, 28)
#define SCREEN_CS _PINNUM(0, 30)
#define SCREEN_SCLK _PINNUM(0, 31)
#define SCREEN_MOSI _PINNUM(0, 29)
#define SCREEN_SRAM_CS -1
#define SCREEN_MISO -1
#define SCREEN_BL _PINNUM(1, 11)

// SX1262
#define SX1262_CS _PINNUM(0, 24)
#define SX1262_RST _PINNUM(0, 25)
#define SX1262_SCLK _PINNUM(0, 19)
#define SX1262_MOSI _PINNUM(0, 22)
#define SX1262_MISO _PINNUM(0, 23)
#define SX1262_BUSY _PINNUM(0, 17)
#define SX1262_INT _PINNUM(1, 8)
#define SX1262_DIO0 _PINNUM(0, 22)
#define SX1262_DIO1 _PINNUM(0, 20)
#define SX1262_DIO2 _PINNUM(0, 5)

// Touch
#define TTP223_TOUCH _PINNUM(0, 11)

// Battery
#define BATTERY_VOLTAGE_ADC _PINNUM(0, 4)

// IIC
#define IIC_SDA _PINNUM(0, 26)
#define IIC_SCL _PINNUM(0, 27)

// RTC
#define RTC_INT _PINNUM(0, 16)

// BOOT
#define nRF52840_BOOT _PINNUM(1, 10)

// GPS
#define GPS_WAKEUP _PINNUM(1, 2)
#define GPS_RST _PINNUM(1, 5)
#define GPS_1PPS _PINNUM(1, 4)
#define GPS_UART_RX _PINNUM(1, 9)
#define GPS_UART_TX _PINNUM(1, 8)

//POWER
#define VDD3_3_EN _PINNUM(0, 12)
