/**
 * @file      radio.cpp
 * @author    Lewis He (lewishe@outlook.com)
 * @license   MIT
 * @copyright Copyright (c) 2024  ShenZhen XinYuan Electronic Technology Co., Ltd
 * @date      2024-05-04
 *
 */
#include "utilities.h"
#include "display.h"
#include "radio.h"


#if   defined(USE_SX1262)
static SX1262          radio      = NULL;
#elif defined(USE_SX1268)
static SX1268          radio      = NULL;
#endif


static bool            transmittedFlag = false;
static bool            startListing = true;
static int             transmissionState = 0;
static uint32_t        interval = 0;
static SPIClass        *rfPort    = nullptr;
static uint32_t         radio_sender_interval = 3000;
static uint32_t         txCounter = 0;
static bool             isRadioOnline = false;
extern xQueueHandle     ledHandler;
/***********************************
  _                _____
 | |              |  __ \
 | |        ___   | |__) |   __ _
 | |       / _ \  |  _  /   / _` |
 | |____  | (_) | | | \ \  | (_| |
 |______|  \___/  |_|  \_\  \__,_|

************************************/

void drawSender()
{
    uint8_t val = SENDER_LED;
    Serial.println(__func__);
    display.setFullWindow();
    display.firstPage();
    int16_t tbx, tby; uint16_t tbw, tbh;
    do {
        display.fillScreen(GxEPD_WHITE);
        display.setFont(DEFAULT_FONT);
        uint16_t wh = DEFAULT_FONT_HEIGHT;
        const char *title = "LoRa Sender";
        display.getTextBounds(title, 0, 0, &tbx, &tby, &tbw, &tbh);
        uint16_t utx = ((display.width() - tbw) / 2) - tbx;
        display.setCursor(utx, wh);
        display.print(title);
        display.drawFastHLine(0, 30, display.width(), GxEPD_BLACK);


        if (isRadioOnline) {
            const char *text[] = {"FREQ:",
                                  "BW:",
                                  "PW:",
                                  "DATA:",
                                  "IAT:",
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
                    display.print(LORA_FREQ);
                    display.print("MHz");
                    break;
                case 1:
                    display.print(LORA_BW);
                    display.print("KHz");
                    break;
                case 2:
                    display.print(LORA_TX_POWER);
                    display.print("dBm");
                    break;
                case 3:
                    display.print("N/A");
                    break;
                case 4:
                    display.print(radio_sender_interval);
                    display.print("ms");
                    break;
                default:
                    break;
                }
            }
        } else {
            display.setFont(&FreeMonoBold24pt7b);
            uint16_t wh = FreeMonoBold24pt7b.yAdvance;
            const char *title = "FAILED";
            display.getTextBounds(title, 0, 0, &tbx, &tby, &tbw, &tbh);
            uint16_t utx = ((display.width() - tbw) / 2) - tbx;
            display.setCursor(utx, wh + 45);
            display.print(title);
            val = FAILED_LED;
        }
    } while (display.nextPage());

    xQueueSend(ledHandler, &val, portMAX_DELAY);
}


void drawReceiver()
{
    uint8_t val = RECV_LED;
    Serial.println(__func__);
    display.setFullWindow();
    display.firstPage();
    int16_t tbx, tby; uint16_t tbw, tbh;
    do {
        display.fillScreen(GxEPD_WHITE);
        display.setFont(DEFAULT_FONT);
        uint16_t wh = DEFAULT_FONT_HEIGHT;
        const char *title = "LoRa Receiver";
        display.getTextBounds(title, 0, 0, &tbx, &tby, &tbw, &tbh);
        uint16_t utx = ((display.width() - tbw) / 2) - tbx;
        display.setCursor(utx, wh);
        display.print(title);
        display.drawFastHLine(0, 30, display.width(), GxEPD_BLACK);

        if (isRadioOnline) {
            const char *text[] = {"FREQ:",
                                  "BW:",
                                  "PW:",
                                  "DATA:",
                                  "RSSI:",
                                  "SNR:",
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
                    display.print(LORA_FREQ);
                    display.print("MHz");
                    break;
                case 1:
                    display.print(LORA_BW);
                    display.print("KHz");
                    break;
                case 2:
                    display.print(LORA_TX_POWER);
                    display.print("dBm");
                    break;
                case 3:
                    display.print("N/A");
                    break;
                case 4:
                    display.print("N/A");
                    break;
                default:
                    break;
                }
            }
        } else {
            display.setFont(&FreeMonoBold24pt7b);
            uint16_t wh = FreeMonoBold24pt7b.yAdvance;
            const char *title = "FAILED";
            display.getTextBounds(title, 0, 0, &tbx, &tby, &tbw, &tbh);
            uint16_t utx = ((display.width() - tbw) / 2) - tbx;
            display.setCursor(utx, wh + 45);
            display.print(title);
            val = FAILED_LED;
        }

    } while (display.nextPage());

    xQueueSend(ledHandler, &val, portMAX_DELAY);
}


void setFlag(void)
{
    // we got a packet, set the flag
    transmittedFlag = true;
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
    // carrier frequency:           868.0 MHz
    // bandwidth:                   125.0 kHz
    // spreading factor:            9
    // coding rate:                 7
    // sync word:                   0x12 (private network)
    // output power:                14 dBm
    // current limit:               60 mA
    // preamble length:             8 symbols
    // TCXO voltage:                1.6 V (set to 0 to not use TCXO)
    // regulator:                   DC-DC (set to true to use LDO)
    // CRC:                         enabled
    int state = radio.begin(LORA_FREQ);
    isRadioOnline = state == RADIOLIB_ERR_NONE;
    if (state != RADIOLIB_ERR_NONE) {
        SerialMon.print(("failed, code "));
        SerialMon.println(state);
        return false;
    }

    radio.setBandwidth(LORA_BW);

    radio.setOutputPower(LORA_TX_POWER);

    radio.setSpreadingFactor(LORA_SF);

    radio.setCurrentLimit(140);

    SerialMon.println(" success");
    return true;
}

void sleepLoRa()
{
    radio.sleep();
    rfPort->end();
    delete rfPort;
    rfPort = NULL;
}

void wakeupLoRa()
{
    radio.standby();
}

void drawRadioSender(String payload)
{
    drawBoxStruct_t data[] = {
        {[]() -> bool{return false;}, 65, 48, 100, 24, "%f", "*C"},          //FREQ
        {[]() -> bool{return false;}, 43, 48 + 24, 100, 24, "%f", "%"},      //BW
        {[]() -> bool{return false;}, 43, 48 + 24 * 2, 100, 24, "%f", "hPa"},//TX POWER
        {[]() -> bool{return true;}, 65, 48 + 24 * 3, 120, 24, "%s", ""},   //payload
        {[]() -> bool{return false;}, 54, 48 + 24 * 4, 100, 24, "%s", ""},   //radio_sender_interval
    };

    uint16_t bgColor = GxEPD_WHITE;
    uint16_t textColor = GxEPD_BLACK;
    uint16_t wh = FreeMono9pt7b.yAdvance;
    display.setFont(&FreeMono9pt7b);
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
                display.print(LORA_FREQ);
                display.print("MHz");
                break;
            case 1:
                display.print(LORA_BW);
                display.print("k");
                break;
            case 2:
                display.print(LORA_TX_POWER);
                display.print("dBm");
                break;
            case 3:
                display.print(payload);
                break;
            case 4:
                display.print(radio_sender_interval);
                display.print("ms");
                break;
            default:
                break;
            }
        } while (display.nextPage());
    }
}

void loopSender()
{
    if (millis() - interval <  radio_sender_interval) {
        return;
    }

    if (!isRadioOnline) {
        return;
    }

    String payload = "echo:" + String(txCounter++);

    if (startListing) {

        startListing = false;

        // set the function that will be called
        // when packet transmission is finished
        radio.setPacketSentAction(setFlag);

        // start transmitting the first packet
        Serial.print(F("[SX1262] Sending first packet ... "));
        // you can transmit C-string or Arduino string up to
        // 256 characters long
        transmissionState = radio.startTransmit(payload);
    }

    // check if the previous transmission finished
    if (transmittedFlag) {
        // reset flag
        transmittedFlag = false;

        if (transmissionState == RADIOLIB_ERR_NONE) {
            // packet was successfully sent
            Serial.println(F("transmission finished!"));
            // NOTE: when using interrupt-driven transmit method,
            //       it is not possible to automatically measure
            //       transmission data rate using getDataRate()
            drawRadioSender(payload);
        } else {
            Serial.print(F("failed, code "));
            Serial.println(transmissionState);

        }

        // clean up after transmission is finished
        // this will ensure transmitter is disabled,
        // RF switch is powered down etc.
        radio.finishTransmit();

        // send another one
        Serial.print(F("[SX1262] Sending another packet ... "));

        // you can transmit C-string or Arduino string up to
        // 256 characters long
        transmissionState = radio.startTransmit(payload);

        // you can also transmit byte array up to 256 bytes long
        /*
          byte byteArr[] = {0x01, 0x23, 0x45, 0x67,
                            0x89, 0xAB, 0xCD, 0xEF};
          int state = radio.startTransmit(byteArr, 8);
        */
    }

    interval = millis();
}

void drawRadioReceiver(String payload)
{
    drawBoxStruct_t data[] = {
        {[]() -> bool{return false;}, 65, 48, 100, 24, "%f", "*C"},         //LORA_FREQ
        {[]() -> bool{return false;}, 43, 48 + 24, 100, 24, "%f", "%"},     //LORA_BW
        {[]() -> bool{return false;}, 43, 48 + 24 * 2, 100, 24, "%f", "hPa"}, //LORA_TX_POWER
        {[]() -> bool{return true;}, 65, 48 + 24 * 3, 120, 24, "%s", ""},  //payload
        {[]() -> bool{return true;}, 65, 48 + 24 * 4, 100, 24, "%s", ""},  //getRSSI
        {[]() -> bool{return true;}, 54, 48 + 24 * 5, 100, 24, "%s", ""},  //getSNR
    };

    uint16_t bgColor = GxEPD_WHITE;
    uint16_t textColor = GxEPD_BLACK;
    uint16_t wh = FreeMono9pt7b.yAdvance;
    display.setFont(&FreeMono9pt7b);
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
                display.print(LORA_FREQ);
                display.print("MHz");
                break;
            case 1:
                display.print(LORA_BW);
                display.print("KHz");
                break;
            case 2:
                display.print(LORA_TX_POWER);
                display.print("dBm");
                break;
            case 3:
                display.print(payload);
                break;
            case 4:
                display.print(radio.getRSSI());
                display.print("dBm");
                break;
            case 5:
                display.print(radio.getSNR());
                display.print("dB");
                break;
            default:
                break;
            }
        } while (display.nextPage());
    }
}


void loopReceiver()
{

    if (!isRadioOnline) {
        return;
    }

    // check if the flag is set
    if (!startListing) {
        startListing = true;

        // set the function that will be called
        // when new packet is received
        radio.setPacketReceivedAction(setFlag);


        transmissionState = radio.startReceive();
        if (transmissionState == RADIOLIB_ERR_NONE) {
            Serial.println(F("success!"));
        } else {
            Serial.print(F("failed, code "));
            Serial.println(transmissionState);
            return;
        }
    }

    if (transmittedFlag) {

        // you can read received data as an Arduino String
        String str;
        int state = radio.readData(str);

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


            drawRadioReceiver(str);


        } else if (state == RADIOLIB_ERR_CRC_MISMATCH) {
            // packet was received, but is malformed
            Serial.println(F("CRC error!"));

        } else {

            // some other error occurred
            Serial.print(F("failed, code "));
            Serial.println(state);
        }

        // put module back to listen mode
        radio.startReceive();

        // reset flag
        transmittedFlag = false;
    }

}

