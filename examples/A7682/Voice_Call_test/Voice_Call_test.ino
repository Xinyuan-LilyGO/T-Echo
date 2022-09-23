
#define TINY_GSM_MODEM_SIM7600 //A7682 Compatible with SIM7600 AT instructions
#define TINY_GSM_RX_BUFFER 1024 // Set RX buffer to 1Kb
#define SerialAT Serial1

// See all AT commands, if wanted
#define DUMP_AT_COMMANDS
#define TINY_GSM_USE_GPRS true

// set GSM PIN, if any
#define GSM_PIN ""
#define UART_BAUD   115200

#include <string>
#include "utilities.h"
#include <SPI.h>
#include <Wire.h>

#include <GxEPD.h>
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
#ifdef USING_BMP280
#include <Adafruit_BMP280.h>
#else
#include <Adafruit_BME280.h>
#endif
#include <bluefruit.h>
#include <TinyGPS++.h>
#include <AceButton.h>
using namespace ace_button;
AceButton button(UserButton_Pin);
AceButton Touched(Touch_Pin);

#include <TinyGsmClient.h>
#include <Adafruit_TinyUSB.h> // for Serial


#ifdef DUMP_AT_COMMANDS  // if enabled it requires the streamDebugger lib
#include <StreamDebugger.h>
StreamDebugger debugger(SerialAT, SerialMon);
TinyGsm modem(debugger);
#else
TinyGsm modem(SerialAT);
#endif

uint32_t        blinkMillis = 0;
// Your GPRS credentials, if any
const char apn[]      = "YourAPN";
const char gprsUser[] = "";
const char gprsPass[] = "";

TinyGsmClient client(modem, 0);


SPIClass        *dispPort  = nullptr;
SPIClass        *rfPort    = nullptr;
GxIO_Class      *io        = nullptr;
GxEPD_Class     *display   = nullptr;


#ifdef USING_BMP280
Adafruit_BMP280 bmp;
#else
Adafruit_BME280 bme;
#endif

bool reply = false;
uint8_t rgb = 0;
uint8_t  UserButton_flag;
bool UserButton_Double_flag = false;
char mgsm[25];
char number[] = "+380xxxxxxxxx";//Change the number you want to dial


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



void deviceProbe(TwoWire &t);
void Key1Scan(void);

// Forward reference to prevent Arduino compiler becoming confused.
void button_Handler(AceButton *button, uint8_t eventType, uint8_t /*buttonState*/);
void Touch_callback();
void button_callback();
void button_Double_callback();
void button_LongPress_callback();

/***********************************
  _____ _____  _____ _____  _           __     __
 |  __ \_   _|/ ____|  __ \| |        /\\ \   / /
 | |  | || | | (___ | |__) | |       /  \\ \_/ /
 | |  | || |  \___ \|  ___/| |      / /\ \\   /
 | |__| || |_ ____) | |    | |____ / ____ \| |
 |_____/_____|_____/|_|    |______/_/    \_\_|

************************************/
void enableBacklight(bool en)
{
    digitalWrite(ePaper_Backlight, en);
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
    display->setRotation(3);
    display->fillScreen(GxEPD_WHITE);
    display->setTextColor(GxEPD_BLACK);
    display->setFont(&FreeMonoBold12pt7b);
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



    ButtonConfig *config = ButtonConfig::getSystemButtonConfig();
    config->setEventHandler(button_Handler);



    config->setFeature(ButtonConfig::kFeatureRepeatPress);
    config->setFeature(ButtonConfig::kFeatureDoubleClick);
    config->setFeature(
        ButtonConfig::kFeatureSuppressClickBeforeDoubleClick);
    config->setFeature(ButtonConfig::kFeatureSuppressAfterClick);
    config->setFeature(ButtonConfig::kFeatureSuppressAfterDoubleClick);

    config->setFeature(ButtonConfig::kFeatureLongPress);
    config->setFeature(ButtonConfig::kFeatureSuppressAfterLongPress);


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
    display->update();

    delay(500);

}

void setup()
{
    // Serial.begin(115200);
    // while ( !Serial ) delay(10);
    pinMode(Power_Enable_Pin, OUTPUT);
    digitalWrite(Power_Enable_Pin, HIGH);
    pinMode(ePaper_Backlight, OUTPUT);
    digitalWrite(ePaper_Backlight, LOW);

    pinMode(GreenLed_Pin, OUTPUT);
    pinMode(RedLed_Pin, OUTPUT);
    pinMode(BlueLed_Pin, OUTPUT);


    pinMode(UserButton_Pin, INPUT_PULLUP);
    pinMode(Touch_Pin, INPUT_PULLUP);

    pinMode(A7682_PWR, OUTPUT);
    // A7682  Power off
    digitalWrite(A7682_PWR, HIGH);
    delay(4000);
    digitalWrite(A7682_PWR, LOW);
    delay(4000);

    // A7682  Power on
    digitalWrite(A7682_PWR, LOW);
    delay(100);
    digitalWrite(A7682_PWR, HIGH);
    delay(1000);
    digitalWrite(A7682_PWR, LOW);
    delay(500);
    pinMode(A7682_RI, INPUT_PULLUP);
    pinMode(A7682_DTR, OUTPUT);
    digitalWrite(A7682_DTR, LOW);

    boardInit();
    delay(2000);

    enableBacklight(false);


    SerialAT.setPins(A7682_TXD, A7682_RXD);
    SerialAT.begin(115200, SERIAL_8N1);

    display->setFont(&FreeMonoBold9pt7b);
    display->fillScreen(GxEPD_WHITE);
    display->setFont(&FreeMonoBold12pt7b);
    display->setCursor(15, 25);
    display->print("start  test  ");
    display->update();

    int i = 10;
    SerialMon.println("\nTesting Modem Response...\n");
    SerialMon.println("****");
    while (i) {
        SerialAT.println("AT");
        delay(500);
        if (SerialAT.available()) {
            String r = SerialAT.readString();
            SerialAT.println(r);
            if ( r.indexOf("OK") >= 0 ) {
                reply = true;
                break;;
            }
        }
        delay(500);
        i--;
    }
    SerialMon.println("****\n");

    if (reply) {
        SerialMon.println(F("***********************************************************"));
        SerialMon.println(F(" You can now send AT commands"));
        SerialMon.println(F(" Enter \"AT\" (without quotes), and you should see \"OK\""));
        SerialMon.println(F(" If it doesn't work, select \"Both NL & CR\" in Serial Monitor"));
        SerialMon.println(F(" DISCLAIMER: Entering AT commands without knowing what they do"));
        SerialMon.println(F(" can have undesired consiquinces..."));
        SerialMon.println(F("***********************************************************\n"));
    } else {
        SerialMon.println(F("***********************************************************"));
        SerialMon.println(F(" Failed to connect to the modem! Check the baud and try again."));
        SerialMon.println(F("***********************************************************\n"));
    }


    // while (1) {
    //     while (Serial.available()) {
    //         SerialAT.write(Serial.read());
    //     }
    //     while (SerialAT.available()) {
    //         Serial.write(SerialAT.read());
    //     }
    // }
    display->setFont(&FreeMonoBold12pt7b);
    display->fillScreen(GxEPD_WHITE);
    display->update();



    SerialMon.println("Initializing modem...");
    if (!modem.init()) {
        SerialMon.println("Failed to restart modem, attempting to continue without restarting");
    }

    String name = modem.getModemName();
    delay(500);
    SerialMon.println("Modem Name: " + name);

    String modemInfo = modem.getModemInfo();
    delay(500);


    DBG("Waiting for network...");
    if (!modem.waitForNetwork()) {
        delay(10000);
        return;
    }

    if (modem.isNetworkConnected()) {
        DBG("Network connected");
    }

    delay(3000);

//*******************ring up ,Hang up after 60 s**************************
    modem.sendAT("+CHUP");
    // check for any of the three for simplicity
    if (modem.waitResponse(1000L) != 1) {
        DBG("Disconnect existing call");
    }

    sprintf(mgsm, "D%S;", number );

    delay(200);
    modem.sendAT(GF(mgsm));
    // check for any of the three for simplicity
    if (modem.waitResponse(10000L) != 1) {
        DBG("call faill");
    }

    delay(60000);

    modem.sendAT("+CHUP");
    // check for any of the three for simplicity
    if (modem.waitResponse(1000L) != 1) {
        DBG("Disconnect existing call");
    }

}




void loop()
{
    button.check();
    if (UserButton_flag) {
        SerialMon.println("UserButton_flag");
        if (digitalRead(A7682_RI) == LOW) {
            SerialMon.println("A7682_RI");
            modem.sendAT("A");
        } else {
            modem.sendAT("+CHUP");
        }
        UserButton_flag = 0;
    }


    if (millis() - blinkMillis > 3000) {

        if (digitalRead(A7682_RI) == LOW) {
            String res;
            modem.sendAT("+CLCC");
            if (modem.waitResponse(10000L, res) != 1) {
            }
            String res1 = res.substring(res.indexOf('"'), res.length());
            String res2  = res1.substring(1, res1.indexOf('"', 3 ));
            // SerialMon.print("res:");
            // SerialMon.println(res2);
            display->setCursor(0, 20);
            display->fillScreen(GxEPD_WHITE);
            display->println("RING...");
            display->println("Number:");
            display->print(res2);
            display->update();
        }


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




}


void button_Double_callback()
{
    SerialMon.println("button_Double_callback");
    UserButton_Double_flag = true;
}

void button_LongPress_callback()
{

}

void button_callback()
{
    UserButton_flag = 1;
    SerialMon.println(F(" UserButton kEventReleased"));
}

void Touch_callback()
{

}

void Touch_LongPress_callback()
{
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
            // SerialMon.println(F("UserButton kEventDoubleClicked"));
            button_Double_callback();
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
            SerialMon.println(F("Touch kEventDoubleClicked"));
            break;

        //case AceButton::kEventReleased:
        // case AceButton::kEventLongReleased:
        case AceButton::kEventLongPressed:
            SerialMon.println(F("Touch kEventLongPressed"));
            break;
        }
    }
}


