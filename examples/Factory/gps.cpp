/**
 * @file      gps.cpp
 * @author    Lewis He (lewishe@outlook.com)
 * @license   MIT
 * @copyright Copyright (c) 2024  ShenZhen XinYuan Electronic Technology Co., Ltd
 * @date      2024-05-04
 *
 */

#include "utilities.h"
#include <TinyGPSPlus.h>
#include "display.h"
#include "sensor.h"

#define TIMEZONE        8
static TinyGPSPlus      *gps;
static uint32_t         interval = 0;
static uint32_t         prevCharsProcessed = 0;
static bool             syncDateTime = false;
String                  gpsVersion = "";

/***********************************
   _____   _____     _____
  / ____| |  __ \   / ____|
 | |  __  | |__) | | (___
 | | |_ | |  ___/   \___ \
 | |__| | | |       ____) |
  \_____| |_|      |_____/

************************************/

#define GNSS_DISABLE_NMEA_OUTPUT "$PCAS03,0,0,0,0,0,0,0,0,0,0,,,0,0*02\r\n"
#define GNSS_GET_VERSION         "$PCAS06,0*1B\r\n"

bool waitResponse(String &data, String rsp, uint32_t timeout)
{
    uint32_t startMillis = millis();
    do {
        while (SerialGPS.available() > 0) {
            int8_t ch = SerialGPS.read();
            data += static_cast<char>(ch);
            if (rsp.length() && data.endsWith(rsp)) {
                return true;
            }
        }
    } while (millis() - startMillis < 1000);
    return false;
}

bool checkOutput()
{
    uint32_t startTimeout = millis() + 500;
    while (SerialGPS.available()) {
        int ch = SerialGPS.read();
        Serial.write(ch);
        if (millis() > startTimeout) {
            SerialGPS.flush();
            Serial.println("Wait L76K stop output timeout!");
            return true;
        }
    };
    SerialGPS.flush();
    return false;
}

bool gnss_probe()
{
    uint8_t retry = 5;

    while (retry--) {

        SerialGPS.write(GNSS_DISABLE_NMEA_OUTPUT);
        delay(5);
        if (checkOutput()) {
            Serial.println("GPS OUT PUT NOT DISABLE .");
            delay(500);
            continue;
        }
        delay(200);
        SerialGPS.write(GNSS_GET_VERSION);
        if (waitResponse(gpsVersion, "$GPTXT,01,01,02", 1000)) {
            Serial.println("L76K GNSS init succeeded, using L76K GNSS Module\n");
            return true;
        }
        delay(500);
    }
    return false;
}


bool gps_probe()
{
    if (!gnss_probe()) {
        return false;
    }
    // Initialize the L76K Chip, use GPS + GLONASS
    SerialGPS.write("$PCAS04,5*1C\r\n");
    delay(250);
    // only ask for RMC and GGA
    SerialGPS.write("$PCAS03,1,0,0,0,1,0,0,0,0,0,,,0,0*02\r\n");
    delay(250);
    // Switch to Vehicle Mode, since SoftRF enables Aviation < 2g
    SerialGPS.write("$PCAS11,3*1E\r\n");
    return true;
}

bool setupGPS()
{
    SerialMon.println("[GPS] Initializing ... ");
    SerialMon.flush();
#ifndef PCA10056
    SerialGPS.setPins(Gps_Rx_Pin, Gps_Tx_Pin);
#endif
    SerialGPS.begin(9600);
    SerialGPS.flush();

    pinMode(Gps_pps_Pin, INPUT);

    pinMode(Gps_Wakeup_Pin, OUTPUT);
    digitalWrite(Gps_Wakeup_Pin, HIGH);

    delay(10);
    pinMode(Gps_Reset_Pin, OUTPUT);
    digitalWrite(Gps_Reset_Pin, HIGH); delay(10);
    digitalWrite(Gps_Reset_Pin, LOW); delay(10);
    digitalWrite(Gps_Reset_Pin, HIGH);
    gps = new TinyGPSPlus();
    return gps_probe();
}

void displayInfo(uint16_t x, uint16_t y, uint16_t w, uint16_t h, uint16_t base_height, uint16_t wh, String content)
{
    uint16_t bgColor = GxEPD_WHITE;
    uint16_t textColor = GxEPD_BLACK;

    display.setPartialWindow(x, y, w, h);
    display.firstPage();
    do {
        display.fillRect(x, y, w, h, bgColor);
        display.setTextColor(textColor);
        display.setCursor(x, base_height + wh);
        display.print(content);
    } while (display.nextPage());
}

drawBoxStruct_t gps_data[] = {
    {
        // charsProcessed
        []() -> bool {
            return true;
        }, 43, 48, 100, 24, "%f", "*C"
    },
    {
        // DATE
        []() -> bool {
            return ( gps->time.isUpdated() && gps->date.isValid() && !(gps->date.year() < 2024) && gps->date.month() <= 12 && gps->date.day() != 0);
        }, 65, 48 + 24, 100, 24, "%f", "%"
    },
    {
        //TIME
        []() -> bool {
            return (gps->time.isUpdated() && gps->time.isValid() && gps->date.isValid() && !(gps->date.year() < 2024) && gps->date.month() <= 12);
        }, 65, 48 + 24 * 2, 100, 24, "%f", "hPa"
    },
    {
        // location
        []() -> bool {
            return gps->location.isValid() && gps->location.isUpdated();
        }, 54, 48 + 24 * 3, 120, 24, "%s", ""
    },
    {
        // location
        []() -> bool {
            return gps->location.isValid();
        }, 54, 48 + 24 * 4, 100, 24, "%s", ""
    },
    {
        // satellites
        []() -> bool {
            return gps->satellites.isValid() && gps->satellites.isUpdated();
        }, 131, 48 + 24 * 5, 100, 24, "%s", ""
    },
};

void drawGPS()
{
    Serial.println(__func__);
    display.setFullWindow();
    display.firstPage();
    int16_t tbx, tby; uint16_t tbw, tbh;
    do {

        display.fillScreen(GxEPD_WHITE);

        display.setFont(DEFAULT_FONT);
        uint16_t wh = DEFAULT_FONT_HEIGHT;

        const char *title = "GPS";
        display.getTextBounds(title, 0, 0, &tbx, &tby, &tbw, &tbh);
        uint16_t utx = ((display.width() - tbw) / 2) - tbx;

        display.setCursor(utx, wh);
        display.print(title);
        display.drawFastHLine(0, 30, display.width(), GxEPD_BLACK);

        const char *text[] = {"RX:",
                              "DATE:",
                              "TIME:",
                              "LNG:",
                              "LAT:",
                              "SATELLITES:"
                             };
        for (uint32_t i = 1; i <= COUNT_SIZE(text); ++i) {

            int tmp_y = 42 + (24 * i);
            display.setCursor(5, tmp_y);
            display.print(text[i - 1]);

            int x =  display.getCursorX();
            int y = display.getCursorY();
            x += 5;
            Serial.printf("i:%d X:%d Y:%d\n", i - 1, x, y);
            display.setCursor(x, y);

            if (!gps_data[i - 1].isUpdate()) {
                display.print("N.A");
                continue;
            }

            switch (i - 1) {
            case 0:
                display.print(gps->charsProcessed());
                break;
            case 1:
                display.print(gps->date.year());
                display.print("/");
                display.print(gps->date.month());
                display.print("/");
                display.print(gps->date.day());
                break;
            case 2:
                display.print(gps->time.hour());
                display.print(":");
                display.print(gps->time.minute());
                display.print(":");
                display.print(gps->time.second());
                break;
            case 3:
                display.print(gps->location.lng(), 6);
                break;
            case 4:
                display.print(gps->location.lat(), 6);
                break;
            case 5:
                display.print(gps->satellites.value());
                break;
            default:
                break;
            }
        }
    } while (display.nextPage());

    extern xQueueHandle ledHandler;
    uint8_t val = PPS_LED;
    xQueueSend(ledHandler, &val, portMAX_DELAY);
}

void loopGPS()
{
    while (SerialGPS.available()) {
        gps->encode(SerialGPS.read());
    }

    uint32_t charsProcessed = gps->charsProcessed();

    if (millis() - interval > 5000) {

        uint16_t bgColor = GxEPD_WHITE;
        uint16_t textColor = GxEPD_BLACK;
        uint16_t wh = DEFAULT_FONT_HEIGHT;

        display.setFont(DEFAULT_FONT);
        display.setTextColor(GxEPD_BLACK);
        static uint32_t updateRxIntervale;

        if (charsProcessed != prevCharsProcessed) {
            prevCharsProcessed = charsProcessed;
            if (millis() < updateRxIntervale) {
                return;
            }

            for (size_t i = 0; i < COUNT_SIZE(gps_data); ++i) {

                if (!gps_data[i].isUpdate()) {
                    continue;
                }
                display.setPartialWindow(gps_data[i].start_x, gps_data[i].start_y, gps_data[i].start_w, gps_data[i].start_h);

                display.firstPage();
                do {
                    drawBox(gps_data[i].start_x, gps_data[i].start_y, gps_data[i].start_w, gps_data[i].start_h, bgColor);
                    display.setTextColor(textColor);

                    display.setCursor(gps_data[i].start_x, gps_data[i].start_y + wh);
                    switch (i) {
                    case 0:
                        if (gps->charsProcessed() < 10) {
                            display.print("No DATA");
                        } else {
                            display.print(charsProcessed);
                        }
                        break;
                    case 1:
                        display.print(gps->date.year());
                        display.print("/");
                        display.print(gps->date.month());
                        display.print("/");
                        display.print(gps->date.day());
                        break;
                    case 2:
                        display.print(gps->time.hour());
                        display.print(":");
                        display.print(gps->time.minute());
                        display.print(":");
                        display.print(gps->time.second());

                        if (!syncDateTime) {
                            syncDateTime = true;
                            rtc.setDateTime(gps->date.year(),
                                            gps->date.month(),
                                            gps->date.day(),
                                            gps->time.hour(),
                                            gps->time.minute(),
                                            gps->time.second()
                                           );
                            Serial.println("SYNC GPS DATE TIME");

                        }
                        break;
                    case 3:
                        display.print(gps->location.lng(), 6);
                        break;
                    case 4:
                        display.print(gps->location.lat(), 6);
                        break;
                    case 5:
                        display.print(gps->satellites.value());
                        break;
                    default:
                        break;
                    }
                } while (display.nextPage());
            }
        }
        if (gps->charsProcessed() < 10) {
            SerialMon.println(F("WARNING: No GPS data.  Check wiring."));
        }
        interval = millis();
    }
}


void sleepGPS()
{
    SerialGPS.end();
    pinMode(Gps_Wakeup_Pin, OUTPUT);
    digitalWrite(Gps_Wakeup_Pin, LOW);
    pinMode(Gps_Reset_Pin, INPUT);
    pinMode(Gps_pps_Pin, INPUT);
    pinMode(Gps_Rx_Pin, INPUT);
    pinMode(Gps_Tx_Pin, INPUT);
}

void wakeupGPS()
{
}


