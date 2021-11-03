/*
GPS Test example

This example is to test GPS functionality.You need to test it outdoors.
The GPS information is displayed on the serial port.

 This example code is in the public domain.
 */
#include "utilities.h"
#include <SPI.h>
#include <Wire.h>

#include <GxEPD.h>
//#include <GxGDEP015OC1/GxGDEP015OC1.h>    // 1.54" b/w
//#include <GxGDEH0154D67/GxGDEH0154D67.h>  // 1.54" b/w
#include <GxDEPG0150BN/GxDEPG0150BN.h>  // 1.54" b/w

#include GxEPD_BitmapExamples
#include <Fonts/FreeMonoBold12pt7b.h>
#include <GxIO/GxIO_SPI/GxIO_SPI.h>
#include <GxIO/GxIO.h>

#include <TinyGPS++.h>



SPIClass        *dispPort  = nullptr;
SPIClass        *rfPort    = nullptr;
GxIO_Class      *io        = nullptr;
GxEPD_Class     *display   = nullptr;

TinyGPSPlus     *gps;

uint32_t        blinkMillis = 0;
uint32_t        last = 0;

uint8_t rgb = 0;

void setupDisplay();
bool setupGPS();
void loopGPS();
void configVDD(void);
void boardInit();
void LilyGo_logo();
void enableBacklight(bool en);

void setup()
{
    Serial.begin(115200);
    delay(200);
    boardInit();
    delay(200);
    LilyGo_logo();
}


void loop()
{
    if (millis() - blinkMillis > 1000) {
        blinkMillis = millis();
        switch (rgb) {
        case 0:
            digitalWrite(GreenLed_Pin, LOW);
            digitalWrite(RedLed_Pin, HIGH);
            digitalWrite(BlueLed_Pin, HIGH);
            break;
        case 1:
            digitalWrite(GreenLed_Pin, HIGH);
            digitalWrite(RedLed_Pin, LOW);
            digitalWrite(BlueLed_Pin, HIGH);
            break;
        case 2:
            digitalWrite(GreenLed_Pin, HIGH);
            digitalWrite(RedLed_Pin, HIGH);
            digitalWrite(BlueLed_Pin, LOW);
            break;
        default :
            break;
        }
        rgb++;
        rgb %= 3;
    }

    loopGPS();

}

void setupDisplay()
{
    dispPort = new SPIClass(
        /*SPIPORT*/NRF_SPIM2,
        /*MISO*/ ePaper_Miso,
        /*SCLK*/ePaper_Sclk,
        /*MOSI*/ePaper_Mosi);

    io = new GxIO_Class(
        *dispPort,
        /*CS*/ ePaper_Cs,
        /*DC*/ ePaper_Dc,
        /*RST*/ePaper_Rst);

    display = new GxEPD_Class(
        *io,
        /*RST*/ ePaper_Rst,
        /*BUSY*/ ePaper_Busy);

    dispPort->begin();
    display->init(/*115200*/);
    display->setRotation(0);
    display->fillScreen(GxEPD_WHITE);
    display->setTextColor(GxEPD_BLACK);
    display->setFont(&FreeMonoBold12pt7b);
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

    return true;
}

void loopGPS()
{
    while (SerialGPS.available() > 0)
        gps->encode(SerialGPS.read());

    if (millis() - last > 5000) {
        if (gps->location.isUpdated()) {
            SerialMon.print(F("LOCATION   Fix Age="));
            SerialMon.print(gps->location.age());
            SerialMon.print(F("ms Raw Lat="));
            SerialMon.print(gps->location.rawLat().negative ? "-" : "+");
            SerialMon.print(gps->location.rawLat().deg);
            SerialMon.print("[+");
            SerialMon.print(gps->location.rawLat().billionths);
            SerialMon.print(F(" billionths],  Raw Long="));
            SerialMon.print(gps->location.rawLng().negative ? "-" : "+");
            SerialMon.print(gps->location.rawLng().deg);
            SerialMon.print("[+");
            SerialMon.print(gps->location.rawLng().billionths);
            SerialMon.print(F(" billionths],  Lat="));
            SerialMon.print(gps->location.lat(), 6);
            SerialMon.print(F(" Long="));
            SerialMon.println(gps->location.lng(), 6);
        }

        else if (gps->date.isUpdated()) {
            SerialMon.print(F("DATE       Fix Age="));
            SerialMon.print(gps->date.age());
            SerialMon.print(F("ms Raw="));
            SerialMon.print(gps->date.value());
            SerialMon.print(F(" Year="));
            SerialMon.print(gps->date.year());
            SerialMon.print(F(" Month="));
            SerialMon.print(gps->date.month());
            SerialMon.print(F(" Day="));
            SerialMon.println(gps->date.day());
        }

        else if (gps->time.isUpdated()) {
            SerialMon.print(F("TIME       Fix Age="));
            SerialMon.print(gps->time.age());
            SerialMon.print(F("ms Raw="));
            SerialMon.print(gps->time.value());
            SerialMon.print(F(" Hour="));
            SerialMon.print(gps->time.hour());
            SerialMon.print(F(" Minute="));
            SerialMon.print(gps->time.minute());
            SerialMon.print(F(" Second="));
            SerialMon.print(gps->time.second());
            SerialMon.print(F(" Hundredths="));
            SerialMon.println(gps->time.centisecond());
        }

        else if (gps->speed.isUpdated()) {
            SerialMon.print(F("SPEED      Fix Age="));
            SerialMon.print(gps->speed.age());
            SerialMon.print(F("ms Raw="));
            SerialMon.print(gps->speed.value());
            SerialMon.print(F(" Knots="));
            SerialMon.print(gps->speed.knots());
            SerialMon.print(F(" MPH="));
            SerialMon.print(gps->speed.mph());
            SerialMon.print(F(" m/s="));
            SerialMon.print(gps->speed.mps());
            SerialMon.print(F(" km/h="));
            SerialMon.println(gps->speed.kmph());
        }

        else if (gps->course.isUpdated()) {
            SerialMon.print(F("COURSE     Fix Age="));
            SerialMon.print(gps->course.age());
            SerialMon.print(F("ms Raw="));
            SerialMon.print(gps->course.value());
            SerialMon.print(F(" Deg="));
            SerialMon.println(gps->course.deg());
        }

        else if (gps->altitude.isUpdated()) {
            SerialMon.print(F("ALTITUDE   Fix Age="));
            SerialMon.print(gps->altitude.age());
            SerialMon.print(F("ms Raw="));
            SerialMon.print(gps->altitude.value());
            SerialMon.print(F(" Meters="));
            SerialMon.print(gps->altitude.meters());
            SerialMon.print(F(" Miles="));
            SerialMon.print(gps->altitude.miles());
            SerialMon.print(F(" KM="));
            SerialMon.print(gps->altitude.kilometers());
            SerialMon.print(F(" Feet="));
            SerialMon.println(gps->altitude.feet());
        }

        else if (gps->satellites.isUpdated()) {
            SerialMon.print(F("SATELLITES Fix Age="));
            SerialMon.print(gps->satellites.age());
            SerialMon.print(F("ms Value="));
            SerialMon.println(gps->satellites.value());
        }

        else if (gps->hdop.isUpdated()) {
            SerialMon.print(F("HDOP       Fix Age="));
            SerialMon.print(gps->hdop.age());
            SerialMon.print(F("ms Value="));
            SerialMon.println(gps->hdop.value());
        }

        if (gps->charsProcessed() < 10)
            Serial.println(F("WARNING: No GPS data.  Check wiring."));
        last = millis();
        Serial.println();
    }
}



void configVDD(void)
{
    // Configure UICR_REGOUT0 register only if it is set to default value.
    if ((NRF_UICR->REGOUT0 & UICR_REGOUT0_VOUT_Msk) ==
            (UICR_REGOUT0_VOUT_DEFAULT << UICR_REGOUT0_VOUT_Pos)) {
        NRF_NVMC->CONFIG = NVMC_CONFIG_WEN_Wen;
        while (NRF_NVMC->READY == NVMC_READY_READY_Busy) {}

        NRF_UICR->REGOUT0 = (NRF_UICR->REGOUT0 & ~((uint32_t)UICR_REGOUT0_VOUT_Msk)) |
                            (UICR_REGOUT0_VOUT_3V3 << UICR_REGOUT0_VOUT_Pos);

        NRF_NVMC->CONFIG = NVMC_CONFIG_WEN_Ren;
        while (NRF_NVMC->READY == NVMC_READY_READY_Busy) {}

        // System reset is needed to update UICR registers.
        NVIC_SystemReset();
    }
}
void boardInit()
{
    uint8_t rlst = 0;

#ifdef HIGH_VOLTAGE
    configVDD();
#endif

    SerialMon.begin(MONITOR_SPEED);
    SerialMon.println("Start\n");

    uint32_t reset_reason;
    sd_power_reset_reason_get(&reset_reason);
    SerialMon.print("sd_power_reset_reason_get:");
    SerialMon.println(reset_reason, HEX);

    pinMode(Power_Enable_Pin, OUTPUT);
    digitalWrite(Power_Enable_Pin, HIGH);

    pinMode(ePaper_Backlight, OUTPUT);
    enableBacklight(false);
    //digitalWrite(ePaper_Backlight, HIGH);

    pinMode(GreenLed_Pin, OUTPUT);
    pinMode(RedLed_Pin, OUTPUT);
    pinMode(BlueLed_Pin, OUTPUT);


    pinMode(UserButton_Pin, INPUT_PULLUP);
    pinMode(Touch_Pin, INPUT_PULLUP);

    int i = 10;
    while (i--) {
        digitalWrite(GreenLed_Pin, !digitalRead(GreenLed_Pin));
        digitalWrite(RedLed_Pin, !digitalRead(RedLed_Pin));
        digitalWrite(BlueLed_Pin, !digitalRead(BlueLed_Pin));
        delay(300);
    }
    digitalWrite(GreenLed_Pin, HIGH);
    digitalWrite(RedLed_Pin, HIGH);
    digitalWrite(BlueLed_Pin, HIGH);

    setupDisplay();
    setupGPS();

    display->update();
    delay(500);


}

void enableBacklight(bool en)
{
    digitalWrite(ePaper_Backlight, en);
}


void LilyGo_logo(void)
{
    display->setRotation(2);
    display->fillScreen(GxEPD_WHITE);
    display->drawExampleBitmap(BitmapExample1, 0, 0, GxEPD_WIDTH, GxEPD_HEIGHT, GxEPD_BLACK);
    display->update();
}