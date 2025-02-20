/**
 * @file      pdm.cpp
 * @author    Lewis He (lewishe@outlook.com)
 * @license   MIT
 * @copyright Copyright (c) 2025  ShenZhen XinYuan Electronic Technology Co., Ltd
 * @date      2025-02-20
 *
 */

#include <PDM.h>
#include "utilities.h"
#include "display.h"

static short sampleBuffer[256];
static volatile int samplesRead;
static const int pdm_data   = 6;
static const int pdm_clk    = 8;
static const int pdm_lr_sel = 15;
static int32_t pdm_avg = 0;
static uint32_t        interval = 0;
void onPDMdata()
{
    // query the number of bytes available
    int bytesAvailable = PDM.available();
    // read into the sample buffer
    PDM.read(sampleBuffer, bytesAvailable);
    // 16-bit, 2 bytes per sample
    samplesRead = bytesAvailable / 2;
}

bool beginPDM()
{
    pinMode(pdm_lr_sel, OUTPUT);
    digitalWrite(pdm_lr_sel, LOW);
    PDM.setPins(pdm_data, pdm_clk, -1);
    // configure the data receive callback
    PDM.onReceive(onPDMdata);
    // optionally set the gain, defaults to 20
    PDM.setGain(30);
    // initialize PDM with:
    // - one channel (mono mode)
    // - a 16 kHz sample rate
    if (!PDM.begin(1, 16000)) {
        Serial.println("Failed to start PDM!");
        return false;
    }
    return true;
}


void drawPDM()
{
    display.setFullWindow();
    display.firstPage();
    int16_t tbx, tby; uint16_t tbw, tbh;
    do {
        display.fillScreen(GxEPD_WHITE);
        display.setFont(DEFAULT_FONT);
        uint16_t wh = DEFAULT_FONT_HEIGHT;
        const char *title = "PDM Data";
        display.getTextBounds(title, 0, 0, &tbx, &tby, &tbw, &tbh);
        uint16_t utx = ((display.width() - tbw) / 2) - tbx;
        display.setCursor(utx, wh);
        display.print(title);
        display.drawFastHLine(0, 30, display.width(), GxEPD_BLACK);

        const char *text[] = {"DATA:"};

        for (uint32_t i = 1; i <= COUNT_SIZE(text); ++i) {

            int tmp_y = 42 + (24 * i);
            display.setCursor(5, tmp_y);
            display.print(text[i - 1]);

            int x =  display.getCursorX();
            int y = display.getCursorY();
            x += 5;
            Serial.printf("i:%d X:%d Y:%d\n", i - 1, x, y);
            display.setCursor(x, y);
            switch (i - 1) {
            case 0:

                display.print(pdm_avg);
                // display.print("");
                break;
            default:
                break;
            }
        }

    } while (display.nextPage());

}

void loopPDM()
{
    // wait for samples to be read
    if (samplesRead) {
        pdm_avg = 0;
        // print samples to the serial monitor or plotter
        for (int i = 0; i < samplesRead; i++) {
            // Serial.println(sampleBuffer[i]);
            pdm_avg += sampleBuffer[i];
        }
        pdm_avg /= samplesRead;
        Serial.printf("avg:%d\n", pdm_avg);
        // clear the read count
        samplesRead = 0;
    }

    if (millis() - interval <  500) {
        return;
    }

    drawBoxStruct_t data[] = {
        {[]() -> bool{return (true);}, 65, 48, 100, 24, "%f", "*C"},         //PDM
    };

    uint16_t bgColor = GxEPD_WHITE;
    uint16_t textColor = GxEPD_BLACK;
    uint16_t wh = DEFAULT_FONT_HEIGHT;
    display.setFont(DEFAULT_FONT);
    display.setTextColor(GxEPD_BLACK);

    for (size_t i = 0; i < COUNT_SIZE(data); ++i) {
        if (!data[i].isUpdate()) {
            continue;
        }
        display.setPartialWindow(data[i].start_x, data[i].start_y, data[i].start_w, data[i].start_h);

        display.firstPage();
        do {
            drawBox(data[i].start_x, data[i].start_y, data[i].start_w, data[i].start_h, bgColor);
            display.setTextColor(textColor);

            display.setCursor(data[i].start_x, data[i].start_y + wh);
            switch (i) {
            case 0:
                display.print(pdm_avg);
                break;
            default:
                break;
            }
        } while (display.nextPage());
    }
    interval = millis();

}