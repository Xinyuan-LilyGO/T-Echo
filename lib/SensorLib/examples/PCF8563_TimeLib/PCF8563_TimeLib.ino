/**
 *
 * @license MIT License
 *
 * Copyright (c) 2022 lewis he
 *
 * Permission is hereby granted, free of charge, to any person obtaining a copy
 * of this software and associated documentation files (the "Software"), to deal
 * in the Software without restriction, including without limitation the rights
 * to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
 * copies of the Software, and to permit persons to whom the Software is
 * furnished to do so, subject to the following conditions:
 *
 * The above copyright notice and this permission notice shall be included in all
 * copies or substantial portions of the Software.
 *
 * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
 * IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
 * FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
 * AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
 * LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
 * OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE
 * SOFTWARE.
 *
 * @file      PCF8563_TimeLib.ino
 * @author    Lewis He (lewishe@outlook.com)
 * @date      2022-12-12
 *
 */
#include <Wire.h>
#include <SPI.h>
#include <Arduino.h>
#include "SensorPCF8563.hpp"
#include <time.h>

#ifndef SENSOR_SDA
#define SENSOR_SDA  42
#endif

#ifndef SENSOR_SCL
#define SENSOR_SCL  41
#endif

#ifndef SENSOR_IRQ
#define SENSOR_IRQ  14
#endif

SensorPCF8563 rtc;
uint32_t intervalue;
char buf[64];

void setup()
{
    Serial.begin(115200);
    while (!Serial);

    pinMode(SENSOR_IRQ, INPUT_PULLUP);

    if (!rtc.begin(Wire, SENSOR_SDA, SENSOR_SCL)) {
        Serial.println("Failed to find PCF8563 - check your wiring!");
        while (1) {
            delay(1000);
        }
    }

    // The simplest way to set up
    rtc.setDateTime(2024, 1, 17, 4, 21, 30);

    // Unix tm structure sets the time
    struct tm timeinfo;
    timeinfo.tm_yday = 2025 - 1900; //Counting starts from 1900, so subtract 1900 here
    timeinfo.tm_mon = 1 - 1;        //Months start at 0, so you need to subtract 1.
    timeinfo.tm_mday = 17;
    timeinfo.tm_hour = 4;
    timeinfo.tm_min = 30;
    timeinfo.tm_sec = 30;
    rtc.setDateTime(timeinfo);
}


void loop()
{
    if (millis() - intervalue > 1000) {

        intervalue = millis();

        struct tm timeinfo;
        // Get the time C library structure
        rtc.getDateTime(&timeinfo);

        // Format the output using the strftime function
        // For more formats, please refer to :
        // https://man7.org/linux/man-pages/man3/strftime.3.html

        size_t written = strftime(buf, 64, "%A, %B %d %Y %H:%M:%S", &timeinfo);

        if (written != 0) {
            Serial.println(buf);
        }

        written = strftime(buf, 64, "%b %d %Y %H:%M:%S", &timeinfo);
        if (written != 0) {
            Serial.println(buf);
        }


        written = strftime(buf, 64, "%A, %d. %B %Y %I:%M%p", &timeinfo);
        if (written != 0) {
            Serial.println(buf);
        }
    }
}



