/*
LORA receive Test example

This example is a receive test for Lora. Using lora testing requires two devices to test,
one of which burns the lora receive sample and the other writes the Lora transmit sample.
You can view the output in the serial port.

 This example code is in the public domain.
 */
#include "utilities.h"
#include <SPI.h>

#include <GxEPD.h>
//#include <GxGDEP015OC1/GxGDEP015OC1.h>    // 1.54" b/w
//#include <GxGDEH0154D67/GxGDEH0154D67.h>  // 1.54" b/w
#include <GxDEPG0150BN/GxDEPG0150BN.h>  // 1.54" b/w

#include GxEPD_BitmapExamples
#include <Fonts/FreeMonoBold12pt7b.h>
#include <GxIO/GxIO_SPI/GxIO_SPI.h>
#include <GxIO/GxIO.h>

#include <RadioLib.h>

// Three states: 0, 1, and 2.
int state = 0;

// Change button increments this counter in Edit mode.
int counter = 0;


SPIClass        *dispPort  = nullptr;
SPIClass        *rfPort    = nullptr;
GxIO_Class      *io        = nullptr;
GxEPD_Class     *display   = nullptr;
SX1262          radio      = nullptr;       //SX1262
uint32_t        blinkMillis = 0;
uint8_t rgb = 0;
// flag to indicate that a packet was received
volatile bool receivedFlag = false;


void setFlag(void);
void setupDisplay();
void loopReceiver();
bool setupLoRa();
void boardInit();
void LilyGo_logo(void);

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
    loopReceiver();
}

void LilyGo_logo(void)
{
    display->fillScreen(GxEPD_WHITE);
    display->drawExampleBitmap(BitmapExample1, 0, 0, GxEPD_WIDTH, GxEPD_HEIGHT, GxEPD_BLACK);
    display->update();
}

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
    display->setRotation(2);
    display->fillScreen(GxEPD_WHITE);
    display->setTextColor(GxEPD_BLACK);
    display->setFont(&FreeMonoBold12pt7b);
}

// this function is called when a complete packet
// is received by the module
// IMPORTANT: this function MUST be 'void' type
//            and MUST NOT have any arguments!
void setFlag(void)
{
    // we got a packet, set the flag
    receivedFlag = true;
}

bool setupLoRa()
{
    rfPort = new SPIClass(
        /*SPIPORT*/NRF_SPIM3,
        /*MISO*/ LoRa_Miso,
        /*SCLK*/LoRa_Sclk,
        /*MOSI*/LoRa_Mosi);
    rfPort->begin();

    SPISettings spiSettings;

    radio = new Module(LoRa_Cs, LoRa_Dio1, LoRa_Rst, LoRa_Busy, *rfPort, spiSettings);

    SerialMon.print("[SX1262] Initializing ...  ");
    int state = radio.begin();
    if (state != RADIOLIB_ERR_NONE) {
        SerialMon.print(("failed, code "));
        SerialMon.println(state);
        return false;
    }

    /*
    *   Sets carrier frequency.
    *   SX1262 : Allowed values are in range from 150.0 to 960.0 MHz.
    * * * */

    if (radio.setFrequency(433.0) == RADIOLIB_ERR_INVALID_FREQUENCY) {
        Serial.println(F("Selected frequency is invalid for this module!"));
        while (true);
    }

    /*
    *   Sets LoRa link bandwidth.
    *   SX1262 : Allowed values are 7.8, 10.4, 15.6, 20.8, 31.25, 41.7, 62.5, 125.0, 250.0 and 500.0 kHz.
    * * * */
    if (radio.setBandwidth(125.0) == RADIOLIB_ERR_INVALID_BANDWIDTH) {
        Serial.println(F("Selected bandwidth is invalid for this module!"));
        while (true);
    }


    /*
    * Sets LoRa link spreading factor.
    * SX1262        :  Allowed values range from 5 to 12.
    * * * */
    if (radio.setSpreadingFactor(12) == RADIOLIB_ERR_INVALID_SPREADING_FACTOR) {
        Serial.println(F("Selected spreading factor is invalid for this module!"));
        while (true);
    }

    /*
    * Sets LoRa coding rate denominator.
    * SX1262 : Allowed values range from 5 to 8. Only available in LoRa mode.
    * * * */
    if (radio.setCodingRate(6) == RADIOLIB_ERR_INVALID_CODING_RATE) {
        Serial.println(F("Selected coding rate is invalid for this module!"));
        while (true);
    }

    /*
    * Sets LoRa sync word.
    * SX1262 : Sets LoRa sync word. Only available in LoRa mode.
    * * */
    if (radio.setSyncWord(0xAB) != RADIOLIB_ERR_NONE) {
        Serial.println(F("Unable to set sync word!"));
        while (true);
    }

    /*
    * Sets transmission output power.
    * SX1262        :  Allowed values are in range from -9 to 22 dBm. This method is virtual to allow override from the SX1261 class.
    * * * */
    if (radio.setOutputPower(22) == RADIOLIB_ERR_INVALID_OUTPUT_POWER) {
        Serial.println(F("Selected output power is invalid for this module!"));
        while (true);
    }

    /*
    * Sets current limit for over current protection at transmitter amplifier.
    * SX1262 : Allowed values range from 45 to 140 mA in 5 mA steps and 120 to 240 mA in 10 mA steps.
    * NOTE: set value to 0 to disable overcurrent protection
    * * * */
    if (radio.setCurrentLimit(140) == RADIOLIB_ERR_INVALID_CURRENT_LIMIT) {
        Serial.println(F("Selected current limit is invalid for this module!"));
        while (true);
    }

    /*
    * Sets preamble length for LoRa or FSK modem.
    * SX1262 : Allowed values range from 1 to 65535.
    * * */
    if (radio.setPreambleLength(16) == RADIOLIB_ERR_INVALID_PREAMBLE_LENGTH) {
        Serial.println(F("Selected preamble length is invalid for this module!"));
        while (true);
    }

    // Enables or disables CRC check of received packets.
    if (radio.setCRC(false) == RADIOLIB_ERR_INVALID_CRC_CONFIGURATION) {
        Serial.println(F("Selected CRC is invalid for this module!"));
        while (true);
    }

    // set the function that will be called
    // when packet transmission is finished
    radio.setDio1Action(setFlag);

    // start listening for LoRa packets
    Serial.print(F("Radio Starting to listen ... "));
    state = radio.startReceive();
    if (state == RADIOLIB_ERR_NONE) {
        Serial.println(F("success!"));
    } else {
        Serial.print(F("failed, code "));
        Serial.println(state);
    }


    SerialMon.println(" success");
    return true;


}



void loopReceiver()
{

    // check if the flag is set
    if (receivedFlag) {
        // reset flag
        receivedFlag = false;

        // you can read received data as an Arduino String
        String str;
        int state = radio.readData(str);

        // you can also read received data as byte array
        /*
          byte byteArr[8];
          int numBytes = radio.getPacketLength();
          int state = radio.readData(byteArr, numBytes);
        */

        if (state == RADIOLIB_ERR_NONE) {
            // packet was successfully received
            Serial.println(F("[SX1262] Received packet!"));

            // print data of the packet
            Serial.print(F("[SX1262] Data:\t\t"));
            Serial.println(str);

            // print RSSI (Received Signal Strength Indicator)
            Serial.print(F("[SX1262] RSSI:\t\t"));
            Serial.print(radio.getRSSI());
            Serial.println(F(" dBm"));

            // print SNR (Signal-to-Noise Ratio)
            Serial.print(F("[SX1262] SNR:\t\t"));
            Serial.print(radio.getSNR());
            Serial.println(F(" dB"));

            // print frequency error
            Serial.print(F("[SX1262] Frequency error:\t"));
            Serial.print(radio.getFrequencyError());
            Serial.println(F(" Hz"));

        } else if (state == RADIOLIB_ERR_CRC_MISMATCH) {
            // packet was received, but is malformed
            Serial.println(F("CRC error!"));

        } else {
            // some other error occurred
            Serial.print(F("failed, code "));
            Serial.println(state);

        }
    }
}


void boardInit()
{
    SerialMon.begin(MONITOR_SPEED);
    // delay(5000);
    while (!SerialMon);
    SerialMon.println("Start\n");

    uint32_t reset_reason;
    sd_power_reset_reason_get(&reset_reason);
    SerialMon.print("sd_power_reset_reason_get:");
    SerialMon.println(reset_reason, HEX);

    pinMode(Power_Enable_Pin, OUTPUT);
    digitalWrite(Power_Enable_Pin, HIGH);

    pinMode(ePaper_Backlight, OUTPUT);
    enableBacklight(false);//off Backlight

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
    setupLoRa();
}
