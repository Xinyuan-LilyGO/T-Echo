/**
 * @file      snesor.cpp
 * @author    Lewis He (lewishe@outlook.com)
 * @license   MIT
 * @copyright Copyright (c) 2024  ShenZhen XinYuan Electronic Technology Co., Ltd
 * @date      2024-05-04
 *
 */
#include "utilities.h"
#include "display.h"
#include "sensor.h"
#include <Adafruit_BME280.h>


static Adafruit_BME280 *bme = NULL;
static uint32_t        interval = 0;
static bool            rtcInterrupt = false;
SensorPCF8563          rtc;
static bool isRtcOnline  = false;
#define VBAT_MV_PER_LSB   (0.73242188F)   // 3.0V ADC range and 12-bit ADC resolution = 3000mV/4096

#ifdef NRF52840_XXAA
#define VBAT_DIVIDER      (0.5F)          // 150K + 150K voltage divider on VBAT
#define VBAT_DIVIDER_COMP (2.0F)          // Compensation factor for the VBAT divider
#else
#define VBAT_DIVIDER      (0.71275837F)   // 2M + 0.806M voltage divider on VBAT = (2M / (0.806M + 2M))
#define VBAT_DIVIDER_COMP (1.403F)        // Compensation factor for the VBAT divider
#endif

#define REAL_VBAT_MV_PER_LSB (VBAT_DIVIDER_COMP * VBAT_MV_PER_LSB)


float readVBAT(void)
{
    float raw;

    // Set the analog reference to 3.0V (default = 3.6V)
    analogReference(AR_INTERNAL_3_0);

    // Set the resolution to 12-bit (0..4095)
    analogReadResolution(12); // Can be 8, 10, 12 or 14

    // Let the ADC settle
    delay(1);

    // Get the raw 12-bit, 0..3000mV ADC value
    raw = analogRead(Adc_Pin);

    // Set the ADC back to the default settings
    analogReference(AR_DEFAULT);
    analogReadResolution(10);

    // Convert the raw value to compensated mv, taking the resistor-
    // divider into account (providing the actual LIPO voltage)
    // ADC range is 0..3000mV and resolution is 12-bit (0..4095)
    raw =  raw * REAL_VBAT_MV_PER_LSB;
    return raw;
}

void drawSensor()
{
    Serial.println(__func__);

    sensors_event_t temp_event, pressure_event;
    sensors_event_t humidity_event;
    if (bme) {
        bme->getTemperatureSensor()->getEvent(&temp_event);
        bme->getPressureSensor()->getEvent(&pressure_event);
        bme->getHumiditySensor()->getEvent(&humidity_event);
    }
    char buf[64];
    struct tm timeinfo;
    // Get the time C library structure
    rtc.getDateTime(&timeinfo);

    display.setFullWindow();
    display.firstPage();
    int16_t tbx, tby; uint16_t tbw, tbh;
    do {
        display.fillScreen(GxEPD_WHITE);
        display.setFont(DEFAULT_FONT);
        uint16_t wh = DEFAULT_FONT_HEIGHT;
        const char *title = "Sensor";
        display.getTextBounds(title, 0, 0, &tbx, &tby, &tbw, &tbh);
        uint16_t utx = ((display.width() - tbw) / 2) - tbx;
        display.setCursor(utx, wh);
        display.print(title);
        display.drawFastHLine(0, 30, display.width(), GxEPD_BLACK);

        const char *text[] = {"TEMP:",
                              "HR:",
                              "PR:",
                              "DATE:",
                              "TIME:",
                              "VBAT:"
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
            switch (i - 1) {
            case 0:
                if (bme) {
                    display.print(temp_event.temperature);
                    display.print("*C");
                } else {
                    display.print("N/A");
                }
                break;
            case 1:
                if (bme) {
                    display.print(humidity_event.relative_humidity);
                    display.print("%");
                } else {
                    display.print("N/A");
                }
                break;
            case 2:
                if (bme) {
                    display.print(pressure_event.pressure);
                    display.print("hPa");
                } else {
                    display.print("N/A");
                }
                break;
            case 3:
                strftime(buf, 64, "%Y/%m/%d", &timeinfo);
                display.print(buf);
                break;
            case 4:
                strftime(buf, 64, "%H:%M:%S", &timeinfo);
                display.print(buf);
                break;
            case 5: {
                float vbat_mv = readVBAT();
                if (vbat_mv > 4200) {
                    display.print("USB powered");
                } else {
                    display.print(vbat_mv / 1000.0);
                    display.print("V");
                }
            }
            break;
            default:
                break;
            }
        }

    } while (display.nextPage());
}

/***********************************
  _                    ___   ___   ___
 | |                  |__ \ / _ \ / _ \
 | |__  _ __ ___  _ __   ) | (_) | | | |
 | '_ \| '_ ` _ \| '_ \ / / > _ <| | | |
 | |_) | | | | | | |_) / /_| (_) | |_| |
 |_.__/|_| |_| |_| .__/____|\___/ \___/
                 | |
                 |_|
************************************/
bool setupSensor()
{
    SerialMon.print("[BME280 ] Initializing ...  ");
    bme = new Adafruit_BME280();
    if (bme->begin()) {
        SerialMon.println("success");
        bme->setSampling(Adafruit_BME280::MODE_NORMAL,
                         Adafruit_BME280::SAMPLING_X2,  // temperature
                         Adafruit_BME280::SAMPLING_X16, // pressure
                         Adafruit_BME280::SAMPLING_X1,  // humidity
                         Adafruit_BME280::FILTER_X16,
                         Adafruit_BME280::STANDBY_MS_0_5 );
        return true;
    }
    SerialMon.println("failed");
    delete bme;
    bme = NULL;
    return false;
}

void sleepSensor()
{
    if (!bme)return;
    bme->setSampling(Adafruit_BME280::MODE_SLEEP);
}

void wakeupSensor()
{
    if (!bme)return;
    bme->setSampling(Adafruit_BME280::MODE_NORMAL);
}

void loopSensor()
{
    if (millis() - interval <  1000) {
        return;
    }
    sensors_event_t temp_event, pressure_event;
    sensors_event_t humidity_event;
    if (bme) {
        bme->getTemperatureSensor()->getEvent(&temp_event);
        bme->getPressureSensor()->getEvent(&pressure_event);
        bme->getHumiditySensor()->getEvent(&humidity_event);
    }
    char buf[64];
    struct tm timeinfo;
    // Get the time C library structure
    if (isRtcOnline) {
        rtc.getDateTime(&timeinfo);
    }
    uint16_t bgColor = GxEPD_WHITE;
    uint16_t textColor = GxEPD_BLACK;
    uint16_t wh = DEFAULT_FONT_HEIGHT;
    display.setFont(DEFAULT_FONT);
    display.setTextColor(GxEPD_BLACK);

    drawBoxStruct_t data[] = {
        {[]() -> bool{return (bme != NULL);}, 65, 48, 100, 24, "%f", "*C"},         //TEMP
        {[]() -> bool{return (bme != NULL);}, 43, 48 + 24, 100, 24, "%f", "%"},     //HM
        {[]() -> bool{return (bme != NULL);}, 43, 48 + 24 * 2, 100, 24, "%f", "hPa"}, //PR
        {[]() -> bool{return true;}, 65, 48 + 24 * 3, 120, 24, "%s", ""},  //DATE
        {[]() -> bool{return true;}, 65, 48 + 24 * 4, 100, 24, "%s", ""},  //TIME
        {[]() -> bool{return true;}, 65, 48 + 24 * 5, 130, 24, "%s", ""},  //VBAT
    };

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
                if (bme) {
                    display.print(temp_event.temperature);
                    display.print("*C");
                } else {
                    display.print("N/A");
                }
                break;
            case 1:
                if (bme) {
                    display.print(humidity_event.relative_humidity);
                    display.print("%");
                } else {
                    display.print("N/A");
                }
                break;
            case 2:
                if (bme) {
                    display.print(pressure_event.pressure);
                    display.print("hPa");
                } else {
                    display.print("N/A");
                }
                break;
            case 3:
                if (isRtcOnline) {
                    strftime(buf, 64, "%Y/%m/%d", &timeinfo);
                    display.print(buf);
                } else {
                    display.print("N/A");
                }
                break;
            case 4:
                if (isRtcOnline) {
                    strftime(buf, 64, "%H:%M:%S", &timeinfo);
                    display.print(buf);
                } else {
                    display.print("N/A");
                }
                break;
            case 5: {
                float vbat_mv = readVBAT();
                if (vbat_mv > 4200) {
                    display.print("USB powered");
                } else {
                    display.print(vbat_mv / 1000.0);
                    display.print("V");
                }
            }
            break;
            default:
                break;
            }
        } while (display.nextPage());
    }
    interval = millis();
}


/***********************************
  _____ _______ _____
 |  __ \__   __/ ____|
 | |__) | | | | |
 |  _  /  | | | |
 | | \ \  | | | |____
 |_|  \_\ |_|  \_____|
***********************************/
void rtcInterruptCb()
{
    rtcInterrupt = true;
}

bool setupRTC()
{
    SerialMon.print("[PCF8563] Initializing ...  ");

    pinMode(RTC_Int_Pin, INPUT);
    attachInterrupt(RTC_Int_Pin, rtcInterruptCb, FALLING);

    Wire.begin();

    // deviceProbe(Wire);

    if (!rtc.begin(Wire)) {
        Serial.println("RTC init failed !");
        isRtcOnline = false;
        return false;
    }
    SerialMon.println("success");

    // Determine whether the hardware clock year, month, and day match the internal time of the RTC.
    // If they do not match, it will be updated to the compilation date
    RTC_DateTime compileDatetime =  RTC_DateTime(__DATE__, __TIME__);
    RTC_DateTime hwDatetime = rtc.getDateTime();
    if (compileDatetime.getYear() != hwDatetime.getYear() ||
            compileDatetime.getMonth() != hwDatetime.getMonth() ||
            compileDatetime.getDay() != hwDatetime.getDay()
       ) {
        Serial.println("No match yy:mm:dd . set datetime to compilation date time");
        rtc.setDateTime(compileDatetime);
    }
    isRtcOnline = true;
    return true;
}

