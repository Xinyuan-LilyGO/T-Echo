
#include "utilities.h"
#include <SPI.h>
#include <Wire.h>

#include <GxEPD.h>
//#include <GxGDEP015OC1/GxGDEP015OC1.h>    // 1.54" b/w
//#include <GxGDEH0154D67/GxGDEH0154D67.h>  // 1.54" b/w
#include <GxDEPG0150BN/GxDEPG0150BN.h>  // 1.54" b/w
#include <TinyGPS++.h>

#include <Fonts/FreeMonoBold9pt7b.h>
#include <Fonts/FreeMonoBold12pt7b.h>
#include <Fonts/FreeMonoBold18pt7b.h>
#include <Fonts/FreeMonoBold24pt7b.h>
#include <GxIO/GxIO_SPI/GxIO_SPI.h>
#include <GxIO/GxIO.h>
#include GxEPD_BitmapExamples

#include "SdFat.h"
#include "Adafruit_SPIFlash.h"

#include <pcf8563.h>
#include <RadioLib.h>
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

// Uncomment to run example with custom SPI and SS e.g with FRAM breakout
Adafruit_FlashTransport_QSPI HWFlashTransport(Flash_Sclk,
        Flash_Cs,
        Flash_Mosi,
        Flash_Miso,
        Flash_WP,
        Flash_HOLD);

Adafruit_SPIFlash QSPIFlash (&HWFlashTransport);

static Adafruit_SPIFlash *SPIFlash = &QSPIFlash;
#define ZD25WQ16B                                                             \
  {                                                                            \
    .total_size = (1UL << 21), /* 2 MiB */                                     \
    .start_up_time_us = 12000, .manufacturer_id = 0xBA,                         \
    .memory_type = 0x60, .capacity = 0x15, .max_clock_speed_mhz =33,         \
    .quad_enable_bit_mask = 0x02, .has_sector_protection = false ,    \
    .supports_fast_read = true , .supports_qspi = true ,   \
    .supports_qspi_writes = true, .write_status_register_split = false, \
    .single_status_byte = false , .is_fram = false,                    \
  }
static const SPIFlash_Device_t my_flash_devices[] = {
    ZD25WQ16B,
};
const int flashDevices = 1;

void sleepnRF52840();
void deviceProbe(TwoWire &t);


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
bool            sleepIn = false;
bool            transmittedFlag = false;
bool            rtcInterrupt = false;
uint8_t rgb = 0;

#define KEY_LP_TIME      2000  /* Long press time       unit  ms                  */
#define KEY_DOUBLE_TIME  500   /* If the interval between two presses does not exceed this value, it is a double - click event*/

uint8_t  UserButton_flag;
uint8_t  Touch_flag;

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

void sleepGPS()
{
    // gps->setTrackingMode();
}

void wakeupGPS()
{
    // gps->wakeup();
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
    SerialMon.print("[SENSOR ] Initializing ...  ");
    // Serial.println("Return!");
    // return false;
#ifdef USING_BMP280
    if (bmp.begin()) {
        SerialMon.println("success");
        /* Default settings from datasheet. */
        bmp.setSampling(Adafruit_BMP280::MODE_NORMAL,     /* Operating Mode. */
                        Adafruit_BMP280::SAMPLING_X2,     /* Temp. oversampling */
                        Adafruit_BMP280::SAMPLING_X16,    /* Pressure oversampling */
                        Adafruit_BMP280::FILTER_X16,      /* Filtering. */
                        Adafruit_BMP280::STANDBY_MS_500); /* Standby time. */
        return true;
    }
#else
    if (bme.begin()) {
        SerialMon.println("success");
        bme.setSampling(Adafruit_BME280::MODE_NORMAL,
                        Adafruit_BME280::SAMPLING_X2,  // temperature
                        Adafruit_BME280::SAMPLING_X16, // pressure
                        Adafruit_BME280::SAMPLING_X1,  // humidity
                        Adafruit_BME280::FILTER_X16,
                        Adafruit_BME280::STANDBY_MS_0_5 );
        return true;
    }
#endif
    SerialMon.println("failed");
    return false;
}

void sleepSensor()
{
#ifdef USING_BMP280
    bmp.setSampling(Adafruit_BMP280::MODE_SLEEP);
#else
    bme.setSampling(Adafruit_BME280::MODE_NORMAL);
#endif
}

void wakeupSensor()
{
#ifdef USING_BMP280
    bmp.setSampling(Adafruit_BMP280::MODE_SLEEP);
#else
    bme.setSampling(Adafruit_BME280::MODE_NORMAL);
#endif
}


///***********************************
//  ______ _                _____ _    _
// |  ____| |        /\    / ____| |  | |
// | |__  | |       /  \  | (___ | |__| |
// |  __| | |      / /\ \  \___ \|  __  |
// | |    | |____ / ____ \ ____) | |  | |
// |_|    |______/_/    \_\_____/|_|  |_|
//
//************************************/
bool setupFlash()
{

    SerialMon.print("[FLASH ] Initializing ...  ");

    if (SPIFlash->begin(my_flash_devices, flashDevices)) {
        SerialMon.print("JEDEC ID: 0x"); Serial.println(SPIFlash->getJEDECID(), HEX);
        SerialMon.print("Flash size: "); Serial.print(SPIFlash->size() / 1024); Serial.println(" KB");
        return true;
    }

    SerialMon.println("failed");
    return false;
}

void sleepFlash()
{
    SPIFlash->end();
}

void wakeupFlash()
{
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

    int retry = 3;

    // deviceProbe(Wire);

    int ret = 0;
    do {

        //HYM8563 bug!! The first visit may not be able to read correctly
        Wire.beginTransmission(PCF8563_SLAVE_ADDRESS);
        ret = Wire.endTransmission();
        delay(200);

    } while (retry--);

    if (ret != 0) {
        SerialMon.println("failed");
        return false;
    }
    SerialMon.println("success");

    rtc.begin(Wire);

    //*rtc self test*//
    Serial.println("============rtc self test==============");

    uint16_t year = 2021, mon = 4, day = 13, hour = 12, min = 0, sec = 57;
    rtc.disableAlarm();
    rtc.setDateTime(year, mon, day, hour, min, sec);
    rtc.setAlarmByMinutes(1);
    rtc.enableAlarm();

    uint32_t seconds = 0;
    for (;;) {

        if (millis() - last > 1000) {
            Serial.println(rtc.formatDateTime());
            last = millis();
            ++seconds;
            // When it runs 10 times without jumping out, it is judged as an error
            if (seconds >= 10) {
                Serial.println("RTC alarm is unusual !");
                return false;
            }
            Serial.flush();
        }


        if (rtcInterrupt) {
            rtcInterrupt = false;
            rtc.resetAlarm();
            if (year != 2021 || mon != 4 || day != 13 || hour != 12 || min != 0) {
                Serial.println("RTC datetime is unusual !");
                return false;
            }
            Serial.println("RTC alarm is normal !");
            break;
        }
    }
    //*rtc self test end*//

    return true;
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

void setFlag(void)
{
    /* // check if the interrupt is enabled
     if (!enableInterrupt) {
         Serial.println("setFlag  return ");
         return;
     }*/
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
    int state = radio.begin(868.0);
    if (state != ERROR_NONE) {
        SerialMon.print(("failed, code "));
        SerialMon.println(state);
        return false;
    }

    // set the function that will be called
    // when packet transmission is finished
    radio.setDio1Action(setFlag);

    SerialMon.println(" success");
    return true;
}

void sleepLoRa()
{
    radio.sleep();
}

void wakeupLoRa()
{
    radio.standby();
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

    config->setFeature(ButtonConfig::kFeatureRepeatPress);
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

    setupBLE();

    setupDisplay();

    rlst |= setupGPS() ? bit(1) : 0 ;

    rlst |= setupLoRa() ? bit(2) : 0;

    rlst |= setupFlash() ? bit(3) : 0;

    rlst |= setupRTC() ? bit(4) : 0;

    rlst |= setupSensor() ? bit(5) : 0;

    display->setCursor(15, 25);
    display->setFont(&FreeMonoBold9pt7b);
    display->fillScreen(GxEPD_WHITE);
    display->println("ePaper SeftTest");
    display->drawFastHLine(0, display->getCursorY() - 5, display->width(), GxEPD_BLACK);
    display->println();
    display->setFont(&FreeMonoBold12pt7b);
    display->print("[GPS]       ");
    display->println(rlst & bit(1) ? "+" : "-");
    display->print("[SX1262]    ");
    display->println(rlst & bit(2) ? "+" : "-");
    display->print("[FLASH]     ");
    display->println(rlst & bit(3) ? "+" : "-");
    display->print("[PCF8563]   ");
    display->println(rlst & bit(4) ? "+" : "-");
    display->print("[BARO]      ");
    display->println(rlst & bit(5) ? "+" : "-");


    display->update();

    delay(500);



}

void wakeupPeripherals()
{
    wakeupGPS();
    wakeupLoRa();
    wakeupSensor();
    wakeupFlash();
    enableBacklight(true);
    pinMode(GreenLed_Pin, OUTPUT);
    pinMode(RedLed_Pin, OUTPUT);
    pinMode(BlueLed_Pin, OUTPUT);
}

void sleepPeripherals()
{
    sleepGPS();
    sleepFlash();
    sleepLoRa();
    sleepSensor();
    enableBacklight(false);
    Wire.end();
    pinMode(SDA_Pin, INPUT);
    pinMode(SCL_Pin, INPUT);

    rfPort->end();
    pinMode(LoRa_Miso, INPUT);
    pinMode(LoRa_Mosi, INPUT);
    pinMode(LoRa_Sclk, INPUT);
    pinMode(LoRa_Cs, INPUT_PULLUP);
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

    pinMode(Touch_Pin, INPUT_PULLUP_SENSE );

    Serial.end();

    (void) sd_softdevice_is_enabled(&sd_en);

    // Enter System OFF state
    if ( sd_en ) {
        sd_power_system_off();
    } else {
        NRF_POWER->SYSTEMOFF = 1;
    }


}

void setup()
{
    Serial.begin(115200);
    delay(200);
    boardInit();
    delay(2000);
    Serial.println("setup");

    display->setRotation(2);
    display->setFont(&FreeMonoBold12pt7b);
    display->fillScreen(GxEPD_WHITE);
    display->drawExampleBitmap(BitmapExample1, 0, 0, GxEPD_WIDTH, GxEPD_HEIGHT, GxEPD_BLACK);
    display->update();
    display->setRotation(3);
    Serial.println("setup dome");

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


    __SEV();
    __WFE();
    __WFE();




}


void deviceProbe(TwoWire &t)
{
    uint8_t err, addr;
    int nDevices = 0;
    for (addr = 1; addr < 127; addr++) {
        t.beginTransmission(addr);
        err = t.endTransmission();
        if (err == 0) {
            Serial.print("I2C device found at address 0x");
            if (addr < 16)
                Serial.print("0");
            Serial.print(addr, HEX);
            Serial.println(" !");
            nDevices++;
        } else if (err == 4) {
            Serial.print("Unknow error at address 0x");
            if (addr < 16)
                Serial.print("0");
            Serial.println(addr, HEX);
        }
    }
    if (nDevices == 0)
        Serial.println("No I2C devices found\n");
    else
        Serial.println("done\n");
}



void button_LongPress_callback()
{
    sleepnRF52840();

}

void button_callback()
{
    UserButton_flag = 1;

}

void Touch_callback()
{
    Touch_flag = 1;


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

        case AceButton::kEventDoubleClicked:  //双击事件
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

        case AceButton::kEventDoubleClicked://双击事件
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
