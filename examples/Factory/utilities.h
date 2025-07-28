#pragma once


#include <Arduino.h>

#ifndef _PINNUM
#define _PINNUM(port, pin)    ((port)*32 + (pin))
#endif


#define ePaper_Miso         _PINNUM(1,6)
#define ePaper_Mosi         _PINNUM(0,29)
#define ePaper_Sclk         _PINNUM(0,31)
#define ePaper_Cs           _PINNUM(0,30)
#define ePaper_Dc           _PINNUM(0,28)
#define ePaper_Rst          _PINNUM(0,2)
#define ePaper_Busy         _PINNUM(0,3)
#define ePaper_Backlight    _PINNUM(1,11)

#define LoRa_Miso           _PINNUM(0,23)
#define LoRa_Mosi           _PINNUM(0,22)
#define LoRa_Sclk           _PINNUM(0,19)
#define LoRa_Cs             _PINNUM(0,24)
#define LoRa_Rst            _PINNUM(0,25)
#define LoRa_Dio0           _PINNUM(0,22)
#define LoRa_Dio1           _PINNUM(0,20)
#define LoRa_Dio2           //_PINNUM(0,3)
#define LoRa_Dio3           _PINNUM(0,21)
#define LoRa_Dio4           //_PINNUM(0,3)
#define LoRa_Dio5           //_PINNUM(0,3)
#define LoRa_Busy           _PINNUM(0,17)


#define Flash_Cs            _PINNUM(1,15)
#define Flash_Miso          _PINNUM(1,13)
#define Flash_Mosi          _PINNUM(1,12)
#define Flash_Sclk          _PINNUM(1,14)
#define Flash_WP            _PINNUM(0,7)
#define Flash_HOLD          _PINNUM(0,5)

#define Touch_Pin           _PINNUM(0,11)
#define Adc_Pin             _PINNUM(0,4)

#define SDA_Pin             _PINNUM(0,26)
#define SCL_Pin             _PINNUM(0,27)

#define RTC_Int_Pin         _PINNUM(0,16)

#define Gps_Rx_Pin          _PINNUM(1,9)
#define Gps_Tx_Pin          _PINNUM(1,8)


#define Gps_Wakeup_Pin      _PINNUM(1,2)
#define Gps_Reset_Pin       _PINNUM(1,5)
#define Gps_pps_Pin         _PINNUM(1,4)



#define UserButton_Pin      _PINNUM(1,10)


#define Power_Enable_Pin    _PINNUM(0,12)
#define Power_Enable1_Pin   _PINNUM(0,13)



#define GreenLed_Pin        _PINNUM(1,1)
#define RedLed_Pin          _PINNUM(1,3)
#define BlueLed_Pin         _PINNUM(0,14)

#define SerialMon           Serial
#define SerialGPS           Serial2

#define MONITOR_SPEED       115200

// Motor & Buzzer Shield
#define DRV2605_ENABLE_PIN  8
#define BUZZER_PIN          6


#define COUNT_SIZE(x)   (sizeof(x)/sizeof(*x))


typedef struct drawBoxStruct {
    bool (*isUpdate)(void);
    uint16_t start_x;
    uint16_t start_y;
    uint16_t start_w;
    uint16_t start_h;
    const char *fmt;
    const char *uint;
} drawBoxStruct_t;
