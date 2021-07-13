

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
uint8_t rgb = 0;

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
void configVDD(void);
void boardInit();
void wakeupPeripherals();
void sleepPeripherals();
void sleepnRF52840();
void LilyGo_logo();

void setupBLE();
void startAdv(void);
void disconnect_callback(uint16_t conn_handle, uint8_t reason);
void connect_callback(uint16_t conn_handle);

// Forward reference to prevent Arduino compiler becoming confused.
void button_Handler(AceButton *button, uint8_t eventType, uint8_t /*buttonState*/);
void Touch_callback();
void button_callback();
void button_LongPress_callback();
void setup()
{
    Serial.begin(115200);
    delay(200);
    boardInit();

    /*display->setRotation(1);
    display->setFont(&FreeMonoBold12pt7b);
    display->fillScreen(GxEPD_WHITE);
    display->update();*/
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
    /* */
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

    ButtonConfig *config = ButtonConfig::getSystemButtonConfig();
    config->setEventHandler(button_Handler);


    config->setFeature(ButtonConfig::kFeatureRepeatPress);//Repeat Press
    config->setFeature(ButtonConfig::kFeatureLongPress);//Long Press
    config->setFeature(ButtonConfig::kFeatureSuppressAfterLongPress);//After Long Press


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
    setupGPS()  ;

    display->update();
    delay(500);


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

    Serial.end();
    delay(1000);
    systemOff(UserButton_Pin, 0);
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


void LilyGo_logo(void)
{
    display->setRotation(2);
    display->fillScreen(GxEPD_WHITE);
    display->drawExampleBitmap(BitmapExample1, 0, 0, GxEPD_WIDTH, GxEPD_HEIGHT, GxEPD_BLACK);
    display->update();
}