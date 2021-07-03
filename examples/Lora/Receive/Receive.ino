/*
 * nRF52840Allfunction.ino: Demonstrate nRF full-function example
 * Copyright 2020 Lewis he
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

#include GxEPD_BitmapExamples
#include <Fonts/FreeMonoBold9pt7b.h>
#include <Fonts/FreeMonoBold12pt7b.h>
#include <Fonts/FreeMonoBold18pt7b.h>
#include <Fonts/FreeMonoBold24pt7b.h>
#include <GxIO/GxIO_SPI/GxIO_SPI.h>
#include <GxIO/GxIO.h>

#include <SerialFlash.h>
#include <pcf8563.h>
#include <RadioLib.h>
#include <Air530.h>
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



void setFlag(void);
void setupDisplay();
void enableBacklight();
void loopReciver();
void sleepnRF52840();
bool setupLoRa();
void configVDD(void);
void boardInit();
void LilyGo_logo(void);


void setupBLE();
void startAdv(void);
void disconnect_callback(uint16_t conn_handle, uint8_t reason);
void connect_callback(uint16_t conn_handle);
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


// BLE Service
BLEDfu  bledfu;  // OTA DFU service
BLEDis  bledis;  // device information
BLEUart bleuart; // uart over ble
BLEBas  blebas;  // battery



SX1262          radio      = nullptr;       //SX1262
PCF8563_Class   rtc;


uint32_t        blinkMillis = 0;


// flag to indicate that a packet was received
volatile bool receivedFlag = false;

// disable interrupt when it's not needed
volatile bool enableInterrupt = true;



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


void setup()
{
    Serial.begin(115200);
    delay(200);
    boardInit();
    delay(200);
    LilyGo_logo();

}

uint8_t rgb = 0;
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
    loopReciver();



}


void LilyGo_logo(void)
{
    display->setRotation(3);
    display->fillScreen(GxEPD_WHITE);
    display->drawExampleBitmap(BitmapExample1, 0, 0, GxEPD_WIDTH, GxEPD_HEIGHT, GxEPD_BLACK);
    display->update();
}

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
  _                _____
 | |              |  __ \
 | |        ___   | |__) |   __ _
 | |       / _ \  |  _  /   / _` |
 | |____  | (_) | | | \ \  | (_| |
 |______|  \___/  |_|  \_\  \__,_|

************************************/


// this function is called when a complete packet
// is received by the module
// IMPORTANT: this function MUST be 'void' type
//            and MUST NOT have any arguments!
void setFlag(void)  //接收到数据执行的函数
{
    // check if the interrupt is enabled  如果禁用了中断，退出函数
    if (!enableInterrupt) {
        return;
    }

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
    int state = radio.begin(868.0);
    if (state != ERR_NONE) {
        SerialMon.print(("failed, code "));
        SerialMon.println(state);
        return false;
    }



    SerialMon.println(" success");
    return true;


}


void loopReciver()
{


    SerialMon.print(F("[SX1262] Waiting for incoming transmission ... "));

    // you can receive data as an Arduino String
    // NOTE: receive() is a blocking method!
    //       See example ReceiveInterrupt for details
    //       on non-blocking reception method.
    String str;
    int state = radio.receive(str);

    // you can also receive data as byte array
    /*
      byte byteArr[8];
      int state = radio.receive(byteArr, 8);
    */

    if (state == ERR_NONE) {
        // packet was successfully received
        SerialMon.println(F("success!"));

        // print the data of the packet
        SerialMon.print(F("[SX1262] Data:\t\t"));
        SerialMon.println(str);

        // print the RSSI (Received Signal Strength Indicator)
        // of the last received packet
        SerialMon.print(F("[SX1262] RSSI:\t\t"));
        SerialMon.print(radio.getRSSI());
        SerialMon.println(F(" dBm"));

        // print the SNR (Signal-to-Noise Ratio)
        // of the last received packet
        SerialMon.print(F("[SX1262] SNR:\t\t"));
        SerialMon.print(radio.getSNR());
        SerialMon.println(F(" dB"));

    } else if (state == ERR_RX_TIMEOUT) {
        // timeout occurred while waiting for a packet
        SerialMon.println(F("timeout!"));

    } else if (state == ERR_CRC_MISMATCH) {
        // packet was received, but is malformed
        SerialMon.println(F("CRC error!"));

    } else {
        // some other error occurred
        SerialMon.print(F("failed, code "));
        SerialMon.println(state);

    }

}




/**********************************************
  ____  _      ______
 |  _ \| |    |  ____|
 | |_) | |    | |__
 |  _ <| |    |  __|
 | |_) | |____| |____
 |____/|______|______|

************************************************/
// callback invoked when central connects
void connect_callback(uint16_t conn_handle)
{
    // Get the reference to current connection
    BLEConnection *connection = Bluefruit.Connection(conn_handle);
    char central_name[32] = { 0 };
    connection->getPeerName(central_name, sizeof(central_name));
    Serial.print("Connected to ");
    Serial.println(central_name);
}

/**
 * Callback invoked when a connection is dropped
 * @param conn_handle connection where this event happens
 * @param reason is a BLE_HCI_STATUS_CODE which can be found in ble_hci.h
 */
void disconnect_callback(uint16_t conn_handle, uint8_t reason)
{
    (void) conn_handle;
    (void) reason;
    Serial.println();
    Serial.print("Disconnected, reason = 0x"); Serial.println(reason, HEX);
}

void startAdv(void)
{
    // Advertising packet
    Bluefruit.Advertising.addFlags(BLE_GAP_ADV_FLAGS_LE_ONLY_GENERAL_DISC_MODE);
    Bluefruit.Advertising.addTxPower();

    // Include bleuart 128-bit uuid
    Bluefruit.Advertising.addService(bleuart);

    // Secondary Scan Response packet (optional)
    // Since there is no room for 'Name' in Advertising packet
    Bluefruit.ScanResponse.addName();

    /* Start Advertising
     * - Enable auto advertising if disconnected
     * - Interval:  fast mode = 20 ms, slow mode = 152.5 ms
     * - Timeout for fast mode is 30 seconds
     * - Start(timeout) with timeout = 0 will advertise forever (until connected)
     *
     * For recommended advertising interval
     * https://developer.apple.com/library/content/qa/qa1931/_index.html
     */
    Bluefruit.Advertising.restartOnDisconnect(true);
    Bluefruit.Advertising.setInterval(32, 244);    // in unit of 0.625 ms
    Bluefruit.Advertising.setFastTimeout(30);      // number of seconds in fast mode
    Bluefruit.Advertising.start(0);                // 0 = Don't stop advertising after n seconds
}

void setupBLE()
{
    Bluefruit.autoConnLed(false);
    // Config the peripheral connection with maximum bandwidth
    // more SRAM required by SoftDevice
    // Note: All config***() function must be called before begin()
    Bluefruit.configPrphBandwidth(BANDWIDTH_MAX);
    Bluefruit.begin();
    Bluefruit.setTxPower(4);    // Check bluefruit.h for supported values
    Bluefruit.setName("Bluefruit52");
    //Bluefruit.setName(getMcuUniqueID()); // useful testing with multiple central connections
    Bluefruit.Periph.setConnectCallback(connect_callback);
    Bluefruit.Periph.setDisconnectCallback(disconnect_callback);

    // To be consistent OTA DFU should be added first if it exists
    bledfu.begin();
    // Configure and Start Device Information Service
    bledis.setManufacturer("Adafruit Industries");
    bledis.setModel("Bluefruit Feather52");
    bledis.begin();

    // Configure and Start BLE Uart Service
    bleuart.begin();

    // Start BLE Battery Service
    blebas.begin();
    blebas.write(100);

    // Set up and start advertising
    startAdv();

    Serial.println("Please use Adafruit's Bluefruit LE app to connect in UART mode");
    Serial.println("Once connected, enter character(s) that you wish to send");
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

    pinMode(ePaper_Backlight, OUTPUT);
    digitalWrite(ePaper_Backlight, HIGH);

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

    setupBLE();

    setupDisplay();

    setupLoRa();

}
