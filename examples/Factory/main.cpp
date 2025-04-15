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

#include "display.h"
#include "gps.h"
#include "sensor.h"
#include "flash.h"
#include "radio.h"
#include "bleApp.h"
#include <AceButton.h>
#include <nrfx_temp.h>
#include <ICM20948_WE.h>
#include <MPU9250.h>
#include <SensorBHI260AP.hpp>
#include <SensorWireHelper.h>

#define BOSCH_APP30_SHUTTLE_BHI260_FW
#include <BoschFirmware.h>

using namespace ace_button;


void drawSender();
void drawReceiver();
void drawSensor();
void drawGPS();
void deviceProbe(TwoWire &t);
void drawDevProbe();
void drawDevicesInfo();
bool probeIMU20948();
bool probeIMU9250();
bool beginPDM();
void drawPDM();
void loopPDM();
void deinitPins();
void sleepFlash();

AceButton button(UserButton_Pin);
uint32_t        blinkMillis = 0;
uint8_t         funcSelectIndex = 0;
uint8_t         prevFuncSelectIndex = 0xFF;
bool            sleepIn = false;
uint16_t         devices_probe_mask = 0;

static bool gotoSleep = false;
extern String BLE_NAME;
typedef void (*funcLoopCallBackTypedef)(void);
typedef void (*funcFirstCallBackTypedef)(void);
TaskHandle_t led_task_handler;
TaskHandle_t btn_task_handler;
struct _callback {
    funcFirstCallBackTypedef firstFunc;
    funcLoopCallBackTypedef loopFunc;
} LilyGoCallBack[] = {
    {drawDevProbe, NULL},
    {drawDevicesInfo, NULL},
    {drawGPS, loopGPS},
    {drawSensor, loopSensor,},
    {drawSender, loopSender,},
    {drawReceiver, loopReceiver},
    {drawPDM, loopPDM},
};

funcFirstCallBackTypedef prevFirstFunc = NULL;

const uint8_t index_max = sizeof(LilyGoCallBack) / sizeof(LilyGoCallBack[0]);
xSemaphoreHandle semHandle = NULL;

/* There are several ways to create your ICM20948 object:
 * ICM20948_WE myIMU = ICM20948_WE()              -> uses Wire / I2C Address = 0x68
 * ICM20948_WE myIMU = ICM20948_WE(ICM20948_ADDR) -> uses Wire / ICM20948_ADDR
 * ICM20948_WE myIMU = ICM20948_WE(&wire2)        -> uses the TwoWire object wire2 / ICM20948_ADDR
 * ICM20948_WE myIMU = ICM20948_WE(&wire2, ICM20948_ADDR) -> all together
 * ICM20948_WE myIMU = ICM20948_WE(CS_PIN, spi);  -> uses SPI, spi is just a flag, see SPI example
 * ICM20948_WE myIMU = ICM20948_WE(&SPI, CS_PIN, spi);  -> uses SPI / passes the SPI object, spi is just a flag, see SPI example
 */
#define ICM20948_ADDR 0x68
ICM20948_WE IMU = ICM20948_WE(ICM20948_ADDR);
MPU9250 IMU_9250;
SensorBHI260AP bhy;

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

void handleEvent(AceButton *button, uint8_t eventType, uint8_t buttonState)
{
    switch (eventType) {
    case AceButton::kEventLongPressed:
        Serial.println("Enter sleep mode!");
        gotoSleep = true;

        break;
    case AceButton::kEventPressed:
        break;
    case AceButton::kEventReleased:
        break;
    case AceButton::kEventClicked:
        sleepIn = false;
        if (xSemaphoreTake(semHandle, portMAX_DELAY)) {
            xSemaphoreGive(semHandle);
            funcSelectIndex++;
            funcSelectIndex %= index_max;
            Serial.print("funcSelectIndex:");
            Serial.println(funcSelectIndex);
        }
        break;
    }
}




xQueueHandle ledHandler;
uint8_t ledState = LOW;
const uint32_t debounceDelay = 50;
uint32_t lastDebounceTime = 0;

void btn_task(void *)
{
    while (1) {
        button.check();
        delay(10);
    }
}

void led_task(void * )
{
    uint8_t prevVal = 0;
    while (1) {
        uint8_t currVal = 0;


        xQueueReceive(ledHandler, &currVal, ms2tick(30));
        if (prevVal != currVal && currVal != 0) {
            digitalWrite(GreenLed_Pin, HIGH);
            digitalWrite(RedLed_Pin, HIGH);
            digitalWrite(BlueLed_Pin, HIGH);
            prevVal = currVal;
        }
        switch (prevVal) {
        case PPS_LED: {
            // GPS PPS
            int pps = digitalRead(Gps_pps_Pin);
            if (pps == HIGH && (millis() - lastDebounceTime) > debounceDelay) {
                ledState = !ledState;
                if (ledState) {
                    digitalWrite(PPS_LED, LOW);
                } else {
                    digitalWrite(PPS_LED, HIGH);
                }
                lastDebounceTime = millis();
            }
        }
        break;

        case SENDER_LED:
            digitalToggle(SENDER_LED);
            delay(100);
            break;
        case RECV_LED:
            digitalToggle(RECV_LED);
            delay(100);
            break;
        case FAILED_LED:
            digitalWrite(RedLed_Pin, LOW);
            delay(100);
        case CONNECT_LED:
            digitalWrite(CONNECT_LED - 1, LOW);
            delay(100);
            break;
        case DISCONNECT_LED:
            digitalWrite(CONNECT_LED - 1, HIGH);
            delay(100);
            break;
        case WHITE_LED:
            digitalWrite(GreenLed_Pin, LOW);
            digitalWrite(RedLed_Pin, LOW);
            digitalWrite(BlueLed_Pin, LOW);
            delay(100);
            break;
        default:
            break;
        }
    }
}



// this function is called when the temperature measurement is ready
void temp_handler(int32_t raw_temp)
{
    float temp_c = raw_temp / 4.0;
    Serial.print(temp_c);
    Serial.println(" C");
}


void drawDevicesInfo()
{
    Serial.println(__func__);
    display.setFullWindow();
    display.firstPage();
    int16_t tbx, tby; uint16_t tbw, tbh;
    do {

        display.fillScreen(GxEPD_WHITE);

        display.setFont(DEFAULT_FONT);
        uint16_t wh = DEFAULT_FONT_HEIGHT;

        const char *title = "DEVICE INFO";
        display.getTextBounds(title, 0, 0, &tbx, &tby, &tbw, &tbh);
        uint16_t utx = ((display.width() - tbw) / 2) - tbx;

        display.setCursor(utx, wh);
        display.print(title);
        display.drawFastHLine(0, 30, display.width(), GxEPD_BLACK);

        const char *text[] = {
            "BSP LIB   :",
            "BOOTLOADER:",
            "SERIAL No :",
            "BLE NAME  :",
            "FLASH SIZE:",
            "GPS:"
        };

        display.setFont(&FreePuhuiti7pt7b);
        for (uint32_t i = 1; i <= COUNT_SIZE(text); ++i) {

            int tmp_y = 42 + (24 * i);
            display.setCursor(5, tmp_y);
            display.print(text[i - 1]);

            int x =  display.getCursorX();
            int y = display.getCursorY();
            x += 5;
            display.setCursor(x, y);
            switch (i - 1) {
            case 0:
                display.print(ARDUINO_BSP_VERSION);
                break;
            case 1:
                display.print(getBootloaderVersion());
                break;
            case 2:
                display.print(getMcuUniqueID());
                break;
            case 3:
                display.print(BLE_NAME);
                break;
            case 4:
                display.print(getFlashSize());
                display.println("KB");
                break;
            case 5:
                display.print(gpsVersion);
                break;

            default:
                break;
            }
        }
    } while (display.nextPage());

    uint8_t val = WHITE_LED;
    xQueueSend(ledHandler, &val, portMAX_DELAY);
}


void drawDevProbe()
{
    display.setFont(DEFAULT_FONT);
    display.setTextColor(GxEPD_BLACK);
    display.setFullWindow();
    display.firstPage();
    do {
        int16_t tbx, tby; uint16_t tbw, tbh;
        uint16_t wh = DEFAULT_FONT_HEIGHT;
        const char *title = "T-Echo Self Test";
        display.getTextBounds(title, 0, 0, &tbx, &tby, &tbw, &tbh);
        uint16_t utx = ((display.width() - tbw) / 2) - tbx;
        display.setCursor(utx, wh);
        display.println(title);

        display.drawFastHLine(0, display.getCursorY() - 5, display.width(), GxEPD_BLACK);
        display.println();

        // display.setFont(&FreeMonoBold12pt7b);
        // display.setFont(&FreeSans12pt7b);
        // display.setFont(&FreeSansBold12pt7b);
        // display.setFont(&FreeSerifItalic12pt7b);
        display.setFont(&FreeMono12pt7b);
        // display.setFont(&FreePuhuiti12pt7b);

        display.print("[  GPS ] ");
        display.println(devices_probe_mask & bit(1) ? "PASS" : "FAIL");
        display.print("["); display.print(RADIO_TYPE); display.print("] ");
        display.println(devices_probe_mask & bit(2) ? "PASS" : "FAIL");
        display.print("[FLASH ] ");
        display.println(devices_probe_mask & bit(3) ? "PASS" : "FAIL");
        display.print("[PCF8563]");
        display.println(devices_probe_mask & bit(4) ? "PASS" : "FAIL");
        display.print("[BME280 ]");
        display.println(devices_probe_mask & bit(5) ? "PASS" : "FAIL");
        display.print("[SPIFFS ]");
        display.println(devices_probe_mask & bit(6) ? "PASS" : "FAIL");

        if (devices_probe_mask & bit(7)) {
            display.print("[IMU20948]");
            display.println("PASS");
        } else if (devices_probe_mask & bit(8)) {
            display.print("[MPU9250]");
            display.println("PASS");
        } else if (devices_probe_mask & bit(9)) {
            display.print("[BHI260 ]");
            display.println("PASS");
        } else {
            display.print("[IMU]");
            display.println("FAIL");
        }

    } while (display.nextPage());

    uint8_t val = WHITE_LED;
    xQueueSend(ledHandler, &val, portMAX_DELAY);
}


void drawSleep()
{
    display.setFullWindow();
    display.firstPage();
    int16_t tbx, tby; uint16_t tbw, tbh;
    do {
        display.fillScreen(GxEPD_WHITE);
        display.setFont(&FreeMonoBold24pt7b);
        uint16_t wh = FreeMonoBold24pt7b.yAdvance;
        const char *title = "SLEEP";
        display.getTextBounds(title, 0, 0, &tbx, &tby, &tbw, &tbh);
        uint16_t utx = ((display.width() - tbw) / 2) - tbx;
        display.setCursor(utx, wh + 45);
        display.print(title);
    } while (display.nextPage());

}



bool probeIMU9250()
{
    if (!IMU_9250.setup(0x68)) {
        SerialMon.println("MPU9250 init failed!");
        return false;
    }
    return true;
}

bool probeIMU20948()
{
    if (!IMU.init()) {
        SerialMon.println("IMU20948 init failed!");
        return false;
    }
    return true;
}

bool probeBHY260AP()
{
    bhy.setFirmware(bosch_firmware_image, bosch_firmware_size, bosch_firmware_type);
    if (!bhy.begin(Wire, BHI260AP_SLAVE_ADDRESS_L)) {
        SerialMon.print("BHI260 failed to initialize sensor - error code:");
        SerialMon.println(bhy.getError());
        return false;
    }
    SerialMon.println("BHI260AP init successfully!");
    return true;
}

void setup()
{
    Wire.setPins(SDA_Pin, SCL_Pin);
    Wire.begin();

    ledHandler = xQueueCreate(1, 1);
    semHandle = xSemaphoreCreateMutex();
    xSemaphoreGive(semHandle);

    SerialMon.begin(MONITOR_SPEED);
    // while (!SerialMon);
    SerialMon.println("Start\n");

    // SensorWireHelper::dumpDevices(Wire);
    uint32_t reset_reason;
    sd_power_reset_reason_get(&reset_reason);
    SerialMon.print("sd_power_reset_reason_get:0x");
    SerialMon.println(reset_reason, HEX);


    pinMode(Power_Enable_Pin, OUTPUT);
    digitalWrite(Power_Enable_Pin, HIGH);

    pinMode(GreenLed_Pin, OUTPUT);
    pinMode(RedLed_Pin, OUTPUT);
    pinMode(BlueLed_Pin, OUTPUT);

    // Button uses the built-in pull up register.
    pinMode(UserButton_Pin, INPUT_PULLUP);
    // Configure the EventHandler, the ButtonConfig , and enable the HeartBeat
    // event. Lower the HeartBeatInterval from 5000 to 2000 for testing purposes.
    ButtonConfig *buttonConfig = button.getButtonConfig();
    buttonConfig->setEventHandler(handleEvent);
    buttonConfig->setFeature(ButtonConfig::kFeatureClick);
    buttonConfig->setFeature(ButtonConfig::kFeatureLongPress);
    // buttonConfig->setHeartBeatInterval(2000);

    Serial.println("setupBLE...");

    setupBLE();

    setupDisplay();

    devices_probe_mask |= setupGPS() ? bit(1) : 0 ;

    devices_probe_mask |= setupLoRa() ? bit(2) : 0;

    devices_probe_mask |= setupFlash() ? bit(3) : 0;

    devices_probe_mask |= setupRTC() ? bit(4) : 0;

    devices_probe_mask |= setupSensor() ? bit(5) : 0;

    devices_probe_mask |= setupInternalFileSystem() ? bit(6) : 0;

    Wire.beginTransmission(0x68);
    if (Wire.endTransmission() == 0) {
        devices_probe_mask |= probeIMU20948() ? bit(7) : 0 ;
        devices_probe_mask |= probeIMU9250() ? bit(8) : 0 ;
    }

    Wire.beginTransmission(0x28);
    if (Wire.endTransmission() == 0) {
        devices_probe_mask |= probeBHY260AP() ? bit(9) : 0;
    }

    beginPDM();

    pinMode(Touch_Pin, INPUT_PULLDOWN);
    attachInterrupt(Touch_Pin, []() {
        adjustBacklight();
    }, RISING);

    xTaskCreate(led_task, "led", 512 * 2, NULL, TASK_PRIO_LOW, &led_task_handler);
    xTaskCreate(btn_task, "led", 512 * 2, NULL, TASK_PRIO_LOW, &btn_task_handler);
}

void loop()
{
    if (xSemaphoreTake(semHandle, portMAX_DELAY)) {
        if (prevFuncSelectIndex != funcSelectIndex) {
            Serial.println("Run firstFunc");
            prevFuncSelectIndex = funcSelectIndex;
            if (LilyGoCallBack[funcSelectIndex].firstFunc) {
                LilyGoCallBack[funcSelectIndex].firstFunc();
            }
        }
        if (LilyGoCallBack[funcSelectIndex].loopFunc) {
            LilyGoCallBack[funcSelectIndex].loopFunc();
        }
        xSemaphoreGive(semHandle);
    }
    if (gotoSleep) {

        vTaskDelete(led_task_handler);
        vTaskDelete(btn_task_handler);

        sleepFlash();

        drawSleep();

        sleepLoRa();

        sleepSensor();

        sleepGPS();

        Wire.end();

        SPI.end();

        digitalWrite(GreenLed_Pin, HIGH);
        digitalWrite(RedLed_Pin, HIGH);
        digitalWrite(BlueLed_Pin, HIGH);

        // setup your wake-up pins.
        detachInterrupt(Touch_Pin);

        deinitPins();

        // pinMode(UserButton_Pin,  INPUT_PULLUP_SENSE);    // this pin (WAKE_LOW_PIN) is pulled up and wakes up the feather when externally connected to ground.
        // pinMode(Touch_Pin, INPUT_PULLDOWN_SENSE);        // this pin (WAKE_HIGH_PIN) is pulled down and wakes up the feather when externally connected to 3.3v.
        // power down nrf52. sleep current ~ 1.1mA
        // Lower shutdown currents require hardware changes, see examples/Sleep_Display/T-Echo_V1.0_PowerConsumptionTest_BLU939(20240909).pdf
        sd_power_system_off(); // this function puts the whole nRF52 to deep sleep (no Bluetooth).  If no sense pins are setup (or other hardware interrupts), the nrf52 will not wake up.
    }
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

void deinitPins()
{
    const uint8_t pins[] = {
        ePaper_Miso,
        ePaper_Mosi,
        ePaper_Sclk,
        ePaper_Cs,
        ePaper_Dc,
        ePaper_Rst,
        ePaper_Busy,
        ePaper_Backlight,
        LoRa_Miso,
        LoRa_Mosi,
        LoRa_Sclk,
        LoRa_Cs,
        LoRa_Rst,
        LoRa_Dio0,
        LoRa_Dio1,
        LoRa_Dio3,
        LoRa_Busy,
        Flash_Cs,
        Flash_Miso,
        Flash_Mosi,
        Flash_Sclk,
        Flash_WP,
        Flash_HOLD,
        Touch_Pin,
        Adc_Pin,
        RTC_Int_Pin,
        Gps_Rx_Pin,
        Gps_Tx_Pin,
        Gps_Wakeup_Pin,
        Gps_Reset_Pin,
        Gps_pps_Pin,
        UserButton_Pin,

        Power_Enable_Pin,
        Power_Enable1_Pin,
    };
    for (auto pin : pins) {
        pinMode(pin, INPUT_PULLDOWN);
    }
    const uint8_t leds [] = {
        GreenLed_Pin,
        RedLed_Pin,
        BlueLed_Pin,
    };
    for (auto pin : leds) {
        pinMode(pin, INPUT_PULLUP);
    }
}