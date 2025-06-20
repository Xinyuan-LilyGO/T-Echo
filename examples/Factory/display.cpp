/**
 * @file      display.cpp
 * @author    Lewis He (lewishe@outlook.com)
 * @license   MIT
 * @copyright Copyright (c) 2024  ShenZhen XinYuan Electronic Technology Co., Ltd
 * @date      2024-05-04
 *
 */


#include "utilities.h"
#include "display.h"
#include <HardwarePWM.h>


// base class GxEPD2_GFX can be used to pass references or pointers to the display instance as parameter, uses ~1.2k more code
// enable or disable GxEPD2_GFX base class
#define ENABLE_GxEPD2_GFX 0

GxEPD2_BW<GxEPD2_154_D67, GxEPD2_154_D67::HEIGHT>
display(GxEPD2_154_D67(/*CS=5*/ ePaper_Cs, /*DC=*/ ePaper_Dc, /*RST=*/ ePaper_Rst, /*BUSY=*/ ePaper_Busy)); // GDEH0154D67 200x200, SSD1681


static SPIClass        *dispPort  = nullptr;
static uint8_t bri_duty = 15;


/***********************************
  _____ _____  _____ _____  _           __     __
 |  __ \_   _|/ ____|  __ \| |        /\\ \   / /
 | |  | || | | (___ | |__) | |       /  \\ \_/ /
 | |  | || |  \___ \|  ___/| |      / /\ \\   /
 | |__| || |_ ____) | |    | |____ / ____ \| |
 |_____/_____|_____/|_|    |______/_/    \_\_|

************************************/

void adjustBacklight()
{
    bri_duty += 51;     //level 5
    bri_duty %= 255;
    HwPWM0.writePin(ePaper_Backlight, bri_duty, false);
}


void setupDisplay()
{

    HwPWM0.addPin(ePaper_Backlight);
    HwPWM0.setResolution(8);
    HwPWM0.setClockDiv(PWM_PRESCALER_PRESCALER_DIV_1); // freq = 16Mhz
    HwPWM0.begin();
    HwPWM0.writePin(ePaper_Backlight, 51, false);


    dispPort = new SPIClass(
        /*SPIPORT*/NRF_SPIM2,
        /*MISO*/ ePaper_Miso,
        /*SCLK*/ePaper_Sclk,
        /*MOSI*/ePaper_Mosi);

    display.epd2.selectSPI(*dispPort, SPISettings(4000000, MSBFIRST, SPI_MODE0));
    display.init(); 
    display.setRotation(3);

    display.setTextColor(GxEPD_BLACK);
    display.setFont(&FreeMonoBold24pt7b);
    display.setFullWindow();
    display.firstPage();
    int16_t tbx, tby; uint16_t tbw, tbh;
    do {
        display.fillScreen(GxEPD_WHITE);
        const char *title = "BOOTING";
        display.getTextBounds(title, 0, 0, &tbx, &tby, &tbw, &tbh);
        uint16_t utx = ((display.width() - tbw) / 2) - tbx;
        display.setCursor(utx, display.height() / 2);
        display.print(title);
    } while (display.nextPage());
    delay(3000);
}


void drawBox(uint16_t x, uint16_t y, uint16_t w, uint16_t h, uint16_t bgColor)
{
    display.fillRect(x, y + 2, w, h, bgColor);
}
