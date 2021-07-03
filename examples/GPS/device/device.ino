/*
   nRF52840Allfunction.ino: Demonstrate nRF full-function example
   Copyright 2020 Lewis he
*/

/*******************************************************
       ______ ______  _____  _____  _____    ___  _____
       | ___ \|  ___||  ___|/ __  \|  _  |  /   ||  _  |
  _ __  | |_/ /| |_   |___ \ `' / /' \ V /  / /| || |/' |
  | '_ \ |    / |  _|      \ \  / /   / _ \ / /_| ||  /| |
  | | | || |\ \ | |    /\__/ /./ /___| |_| |\___  |\ |_/ /
  |_| |_|\_| \_|\_|    \____/ \_____/\_____/    |_/ \___/

*********************************************************/
#include "utilities.h"
#include <SPI.h>
#include <Wire.h>

#include <GxEPD.h>
//#include <GxGDEP015OC1/GxGDEP015OC1.h>    // 1.54" b/w
//#include <GxGDEH0154D67/GxGDEH0154D67.h>  // 1.54" b/w
#include <GxDEPG0150BN/GxDEPG0150BN.h>  // 1.54" b/w


#include <Fonts/FreeMonoBold9pt7b.h>
#include <Fonts/FreeMonoBold12pt7b.h>
#include <Fonts/FreeMonoBold18pt7b.h>
#include <Fonts/FreeMonoBold24pt7b.h>
#include <GxIO/GxIO_SPI/GxIO_SPI.h>
#include <GxIO/GxIO.h>

#include <SerialFlash.h>
#include <pcf8563.h>
#include <RadioLib.h>
#include <TinyGPS++.h>
#ifdef USING_BMP280
#include <Adafruit_BMP280.h>
#else
#include <Adafruit_BME280.h>
#endif
#include <bluefruit.h>





#include <AceButton.h>
using namespace ace_button;
AceButton button(UserButton_Pin);
AceButton Touched(Touch_Pin);




// Three states: 0, 1, and 2.
int state = 0;

// Change button increments this counter in Edit mode.
int counter = 0;

// LongPress on ModeButton will go into "edit" mode.
bool isEditting = false;

// In edit mode, the "field" is blinking. But when the Change button is
// Pressed or LongPressed, the blinking temporarily stops.
bool isBlinking = false;



void loopGPS();
void loopSensor();
void loopGPS();
void loopSender();
void loopReciver();
void sleepnRF52840();
void deviceProbe(TwoWire &t);
void Key1Scan(void);

// Forward reference to prevent Arduino compiler becoming confused.
/*void button_handleEvent(AceButton *, uint8_t, uint8_t);
  void Touch_handleEvent(AceButton *, uint8_t, uint8_t);*/
void button_Handler(AceButton *button, uint8_t eventType, uint8_t /*buttonState*/);
void Touch_callback();
void button_callback();
void button_DoubleClicked_callback();
void button_LongPress_callback();


/***********************************
   ____  ____       _
  / __ \|  _ \     | |
  | |  | | |_) |    | |
  | |  | |  _ < _   | |
  | |__| | |_) | |__| |
  \____/|____/ \____/

************************************/
SPIClass        *dispPort  = nullptr;
SPIClass        *rfPort    = nullptr;
GxIO_Class      *io        = nullptr;
GxEPD_Class     *display   = nullptr;
#if defined(VERSION_1)
Air530          *gps       = nullptr;
#else
TinyGPSPlus     *gps;
#endif
SX1262          radio      = nullptr;       //SX1262
PCF8563_Class   rtc;
#ifdef USING_BMP280
Adafruit_BMP280 bmp;
#else
Adafruit_BME280 bme;
#endif
// BLE Service
BLEDfu  bledfu;  // OTA DFU service
BLEDis  bledis;  // device information
BLEUart bleuart; // uart over ble
BLEBas  blebas;  // battery

uint32_t        blinkMillis = 0;
uint32_t        last = 0;
uint8_t         funcSelectIndex = 0;
uint8_t         prevFuncSelectIndex = 0;
bool            sleepIn = false;
bool            transmittedFlag = false;
bool            enableInterrupt = true;
bool            startListing = true;
int             transmissionState = 0;
bool            rtcInterrupt = false;





uint8_t  UserButton_flag;
uint8_t  Touch_flag;
uint8_t mode = 0;


const uint8_t index_max = 5;
typedef void (*funcCallBackTypedef)(void);
funcCallBackTypedef LilyGoCallBack[] = { loopSensor, loopGPS,  loopSender, loopReciver, sleepnRF52840};

const char *playload[] = {"0", "1", "2", "3"};

int numtest = 0;

typedef enum {
    nRF52_RESETPIN = 1,         /*Reset from pin-reset detected*/
    nRF52_DOG,                  /*Reset from watchdog detected*/
    nRF52_SREQ,                 /*Reset from soft reset detected*/
    nRF52_LOCKUP,               /*Reset from CPU lock-up detected*/
    nRF52_OFF = bit(16),        /*Reset due to wake up from System OFF mode when wakeup is triggered from DETECT signal from GPIO*/
    nRF52_LPCOMP,               /*Reset due to wake up from System OFF mode when wakeup is triggered from ANADETECT signal from LPCOM*/
    nRF52_DIF,                  /*Reset due to wake up from System OFF mode when wakeup is triggered from entering into debug interface mode*/
    nRF52_NFC,                  /*Reset due to wake up from System OFF mode by NFC field*/
    nRF52_VBUS,                 /*Reset due to wake up from System OFF mode by VBUS rising*/
} nRF52_ResetReason;



void setupDisplay();
bool setupGPS();
void loopGPS();
void wakeupGPS();
void sleepGPS();
void configVDD(void);
void boardInit();
void wakeupPeripherals();
void sleepPeripherals();
void sleepnRF52840();
void displayInfo();


void setup()
{
    Serial.begin(115200);
    delay(200);
    boardInit();
    delay(2000);
    Serial.println("setup");

    display->setRotation(1);
    display->setFont(&FreeMonoBold12pt7b);
    display->fillScreen(GxEPD_WHITE);
    display->update();



}

uint8_t rgb = 0;
void loop()
{
    //   Serial.print("UserButton_Pin=");
    // Serial.println(digitalRead(UserButton_Pin));


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

    button.check();
    Touched.check();


    loopGPS();


}

void button_LongPress_callback()
{
    sleepnRF52840();
    /*  display->fillScreen(GxEPD_WHITE);
      display->setCursor(15, 100);
      display->setFont(&FreeMonoBold12pt7b);
      display->println("button_LongPress_callback");
      display->update();*/
}

void button_callback()
{
    UserButton_flag = 1;
    /*  display->fillScreen(GxEPD_WHITE);
      display->setCursor(15, 100);
      display->setFont(&FreeMonoBold12pt7b);
      display->println("button_callback");
      display->update();*/
}

void Touch_callback()
{
    Touch_flag = 1;
    /*
      display->fillScreen(GxEPD_WHITE);
      display->setCursor(15, 100);
      display->setFont(&FreeMonoBold12pt7b);
      display->println("Touch_callback");
      display->update();*/

}

void Touch_LongPress_callback()
{
    /*    display->fillScreen(GxEPD_WHITE);
        display->setCursor(15, 100);
        display->setFont(&FreeMonoBold12pt7b);
        display->println("Touch_LongPress_callback");
        display->update();*/
}

// The event handler for the buttons.
void button_Handler(AceButton *button, uint8_t eventType, uint8_t /*buttonState*/)
{
    uint8_t pin = button->getPin();

    if (pin == UserButton_Pin) {
        switch (eventType) {
        // Interpret a Released event as a Pressed event, to distiguish it
        // from a LongPressed event.
        case AceButton::kEventReleased:
            button_callback();
            //Serial.println(F(" UserButton kEventReleased"));
            break;

        case AceButton::kEventDoubleClicked:
            Serial.println(F("UserButton kEventDoubleClicked"));
            break;

        // LongPressed goes in and out of edit mode.
        case AceButton::kEventLongPressed:
            button_LongPress_callback();
            //Serial.println(F("UserButton kEventLongPressed"));
            break;
        }
    } else if (pin == Touch_Pin) {
        switch (eventType) {
        //case AceButton::kEventPressed:
        case AceButton::kEventReleased:
            Touch_callback();
            //Serial.println(" Touch kEventReleased");
            break;

        case AceButton::kEventDoubleClicked:
            Serial.println(F("Touch kEventDoubleClicked"));
            break;

        //case AceButton::kEventReleased:
        // case AceButton::kEventLongReleased:
        case AceButton::kEventLongPressed:
            Serial.println(F("Touch kEventLongPressed"));
            break;
        }
    }
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

/***********************************
   _____   _____     _____
  / ____| |  __ \   / ____|
  | |  __  | |__) | | (___
  | | |_ | |  ___/   \___ \
  | |__| | | |       ____) |
  \_____| |_|      |_____/

************************************/
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
#if defined(VERSION_1)
    gps = new Air530(&SerialGPS, Gps_Wakeup_Pin);
#else
    delay(10);
    pinMode(Gps_Reset_Pin, OUTPUT);
    digitalWrite(Gps_Reset_Pin, HIGH); delay(10);
    digitalWrite(Gps_Reset_Pin, LOW); delay(10);
    digitalWrite(Gps_Reset_Pin, HIGH);
    gps = new TinyGPSPlus();
#endif
    return true;
}

void loopGPS()
{
    // This sketch displays information every time a new sentence is correctly encoded.
    while (SerialGPS.available() > 0)
        if (gps->encode(SerialGPS.read()))
            displayInfo();

    if (millis() > 5000 && gps->charsProcessed() < 10) {
        Serial.println(F("No GPS detected: check wiring."));
        while (true);
    }


}


void displayInfo()
{
    Serial.print(F("Location: "));
    if (gps->location.isValid()) {
        Serial.print(gps->location.lat(), 6);
        Serial.print(F(","));
        Serial.print(gps->location.lng(), 6);
    } else {
        Serial.print(F("INVALID"));
    }

    Serial.print(F("  Date/Time: "));
    if (gps->date.isValid()) {
        Serial.print(gps->date.month());
        Serial.print(F("/"));
        Serial.print(gps->date.day());
        Serial.print(F("/"));
        Serial.print(gps->date.year());
    } else {
        Serial.print(F("INVALID"));
    }

    Serial.print(F(" "));
    if (gps->time.isValid()) {
        if (gps->time.hour() < 10) Serial.print(F("0"));
        Serial.print(gps->time.hour());
        Serial.print(F(":"));
        if (gps->time.minute() < 10) Serial.print(F("0"));
        Serial.print(gps->time.minute());
        Serial.print(F(":"));
        if (gps->time.second() < 10) Serial.print(F("0"));
        Serial.print(gps->time.second());
        Serial.print(F("."));
        if (gps->time.centisecond() < 10) Serial.print(F("0"));
        Serial.print(gps->time.centisecond());
    } else {
        Serial.print(F("INVALID"));
    }

    Serial.println();
}

void sleepGPS()
{
    // gps->setTrackingMode();
}

void wakeupGPS()
{
    // gps->wakeup();
}


/***********************************
  __  __           _
  |  \/  |         (_)
  | \  / |   __ _   _   _ __
  | |\/| |  / _` | | | | '_ \
  | |  | | | (_| | | | | | | |
  |_|  |_|  \__,_| |_| |_| |_|

************************************/
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
    // delay(5000);
    // while (!SerialMon);
    SerialMon.println("Start\n");

    uint32_t reset_reason;
    sd_power_reset_reason_get(&reset_reason);
    SerialMon.print("sd_power_reset_reason_get:");
    SerialMon.println(reset_reason, HEX);

    pinMode(Power_Enable_Pin, OUTPUT);
    digitalWrite(Power_Enable_Pin, HIGH);
    /*
      #if defined(Power_Enable1_Pin)
        pinMode(Power_Enable1_Pin, OUTPUT);
        digitalWrite(Power_Enable1_Pin, HIGH);
      #endif
    */
    pinMode(ePaper_Backlight, OUTPUT);
    digitalWrite(ePaper_Backlight, HIGH);

    pinMode(GreenLed_Pin, OUTPUT);
    pinMode(RedLed_Pin, OUTPUT);
    pinMode(BlueLed_Pin, OUTPUT);


    pinMode(UserButton_Pin, INPUT_PULLUP);
    pinMode(Touch_Pin, INPUT_PULLUP);

    ButtonConfig *config = ButtonConfig::getSystemButtonConfig();
    config->setEventHandler(button_Handler);


    config->setFeature(ButtonConfig::kFeatureRepeatPress);//按下释放
    config->setFeature(ButtonConfig::kFeatureLongPress);//长按
    config->setFeature(ButtonConfig::kFeatureSuppressAfterLongPress);//抑制长按后的再次触发按键释放的事件


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


    setupGPS()  ;


    display->update();

    delay(500);

    // sleepnRF52840();
}

void wakeupPeripherals()
{
    wakeupGPS();

    enableBacklight(true);
    pinMode(GreenLed_Pin, OUTPUT);
    pinMode(RedLed_Pin, OUTPUT);
    pinMode(BlueLed_Pin, OUTPUT);
}

void sleepPeripherals()
{
    sleepGPS();

    enableBacklight(false);
    Wire.end();
    pinMode(SDA_Pin, INPUT);
    pinMode(SCL_Pin, INPUT);

    rfPort->end();
    pinMode(LoRa_Miso, INPUT);
    pinMode(LoRa_Mosi, INPUT);
    pinMode(LoRa_Sclk, INPUT);
    pinMode(LoRa_Cs, INPUT);
    pinMode(LoRa_Rst, INPUT);

    dispPort->end();
    pinMode(ePaper_Miso, INPUT);
    pinMode(ePaper_Mosi, INPUT);
    pinMode(ePaper_Sclk, INPUT);
    pinMode(ePaper_Cs, INPUT);
    pinMode(ePaper_Dc, INPUT);
    pinMode(ePaper_Rst, INPUT);
    pinMode(ePaper_Busy, INPUT);

    pinMode(Flash_Cs, INPUT);
    pinMode(Flash_Miso, INPUT);
    pinMode(Flash_Mosi, INPUT);
    pinMode(Flash_Sclk, INPUT);

    digitalWrite(Power_Enable_Pin, LOW);
    pinMode(Power_Enable_Pin, INPUT);
    /*
      #if defined(Power_Enable1_Pin)
        digitalWrite(Power_Enable1_Pin, LOW);
        pinMode(Power_Enable1_Pin, INPUT);
      #endif
    */
    pinMode(GreenLed_Pin, INPUT);
    pinMode(RedLed_Pin, INPUT);
    pinMode(BlueLed_Pin, INPUT);


    pinMode(Gps_Wakeup_Pin, INPUT);
    pinMode(Gps_Rx_Pin, INPUT);
    pinMode(Gps_Tx_Pin, INPUT);
    pinMode(Gps_Wakeup_Pin, INPUT);
    pinMode(Gps_Reset_Pin, INPUT);
    pinMode(Gps_pps_Pin, INPUT);
    pinMode(Adc_Pin, INPUT);


}

void sleepnRF52840()
{
    uint8_t sd_en;
    display->fillScreen(GxEPD_WHITE);
    display->setFont(&FreeMonoBold18pt7b);
    display->setCursor(50, 100);
    display->println("Sleep");
    display->update();
    delay(2000);
    sleepIn = true;
    sleepPeripherals();

    //systemOff(UserButton_Pin, 0);
    // waitForEvent();

    //NRF_POWER->GPREGRET = 0x6d;//DFU_MAGIC_SKIP;
    // pinMode(UserButton_Pin, INPUT_PULLUP_SENSE /* INPUT_SENSE_LOW */);

    Serial.end();
    delay(1000);
    systemOff(Touch_Pin, 0);
    NRF_POWER->SYSTEMOFF = 1;
    while (true);


    /*

      sd_power_dcdc_mode_set(NRF_POWER_DCDC_DISABLE);
      // power down nrf52.
      //sd_power_system_off();       // this function puts the whole nRF52 to deep sleep (no Bluetooth).  If no sense pins are setup (or other hardware interrupts), the nrf52 will not wake up.
      NRF_POWER->SYSTEMOFF = 1;
      // wakeupPeripherals();*/


}

void enableBacklight(bool en)
{
    digitalWrite(ePaper_Backlight, en);
}
