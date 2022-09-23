#define BLYNK_TEMPLATE_ID ""
#define BLYNK_DEVICE_NAME ""
#define BLYNK_AUTH_TOKEN ""

#define TINY_GSM_MODEM_SIM7600 //A7608 Compatible with SIM7600 AT instructions
#define TINY_GSM_RX_BUFFER 1024 // Set RX buffer to 1Kb
#define SerialAT Serial1

// Set serial for debug console (to the Serial Monitor, default speed 115200)
#define SerialMon Serial

// See all AT commands, if wanted
// #define DUMP_AT_COMMANDS

#define SEALEVELPRESSURE_HPA (1013.25)

#define UART_BAUD   115200
#define SerialAT Serial1


#include <TinyGsmClient.h>
#include <Adafruit_TinyUSB.h> // for Serial


#ifdef DUMP_AT_COMMANDS  // if enabled it requires the streamDebugger lib
#include <StreamDebugger.h>
StreamDebugger debugger(SerialAT, SerialMon);
TinyGsm modem(debugger);
#else
TinyGsm modem(SerialAT);
#endif

#include "utilities.h"
#include <SPI.h>
#include <Wire.h>
#include <GxEPD.h>
#include <GxDEPG0150BN/GxDEPG0150BN.h>  // 1.54" b/w 
#include GxEPD_BitmapExamples
#include <Fonts/FreeMonoBold12pt7b.h>
#include <GxIO/GxIO_SPI/GxIO_SPI.h>
#include <GxIO/GxIO.h>

#include <Fonts/FreeMonoBold18pt7b.h>
#include <BlynkSimpleTinyGSM.h>
#include <Adafruit_BME280.h>

SPIClass        *dispPort  = nullptr;
SPIClass        *rfPort    = nullptr;
GxIO_Class      *io        = nullptr;
GxEPD_Class     *display   = nullptr;

uint32_t        blinkMillis = 0;
uint8_t rgb = 0;
bool reply = false;
Adafruit_BME280 bme;

BlynkTimer timer;

// You should get Auth Token in the Blynk App.
// Go to the Project Settings (nut icon).
char auth[] = BLYNK_AUTH_TOKEN;

// Your GPRS credentials
// Leave empty, if missing user or pass
// char apn[]  = "CMNET";
char apn[]  = "YourAPN";
char user[] = "";
char pass[] = "";


void setupDisplay();
void enableBacklight();
void configVDD(void);
void boardInit();
void LilyGo_logo(void);
void loopSensor();
void wakeupSensor();
void sleepSensor();
bool setupSensor();

//Syncing the output state with the app at startup
BLYNK_CONNECTED()
{
    // Blynk.syncVirtual(V3);  // will cause BLYNK_WRITE(V0) to be executed
}

// This function sends Arduino's up time every second to Virtual Pin (5).
// In the app, Widget's reading frequency should be set to PUSH. This means
// that you define how often to send data to Blynk App.
void sendSensor()
{

    display->setCursor(0, 25);
    // display->print("loading...");
    display->updateWindow(0, 0, GxEPD_WIDTH, GxEPD_HEIGHT, false);
    //  if (millis() - last > 3000) {
    // return;


    sensors_event_t temp_event, pressure_event;
#ifdef USING_BMP280
    bmp.getTemperatureSensor()->getEvent(&temp_event);
    bmp.getPressureSensor()->getEvent(&pressure_event);
#else
    sensors_event_t humidity_event;
    bme.getTemperatureSensor()->getEvent(&temp_event);
    bme.getPressureSensor()->getEvent(&pressure_event);
    bme.getHumiditySensor()->getEvent(&humidity_event);

    SerialMon.print("Humidity = ");
    SerialMon.print(humidity_event.relative_humidity);
    SerialMon.println(" %");
#endif
    SerialMon.print(F("Temperature = "));
    SerialMon.print(temp_event.temperature);
    SerialMon.println(" *C");

    SerialMon.print(F("Pressure = "));
    SerialMon.print(pressure_event.pressure);
    SerialMon.println(" hPa");

    SerialMon.println();

    display->fillRect(0, 0, GxEPD_WIDTH, GxEPD_HEIGHT, GxEPD_WHITE);

    display->setCursor(0, 25);
    display->setFont(&FreeMonoBold12pt7b);
    display->println("[Temperature]");

    display->setFont(&FreeMonoBold18pt7b);
    display->setCursor(30, 80);
    display->print(temp_event.temperature);

    display->setFont(&FreeMonoBold12pt7b);
    display->setCursor(display->getCursorX(), display->getCursorY() - 20);
    display->println("*C");

    display->setCursor(0, 120);
    display->println("[Pressure]");

    display->setFont(&FreeMonoBold18pt7b);
    display->setCursor(30, 175);
    display->print(pressure_event.pressure);

    display->setFont(&FreeMonoBold12pt7b);
    display->setCursor(display->getCursorX(), display->getCursorY() - 20);
    display->println("%");

    display->updateWindow(0, 0, GxEPD_WIDTH, GxEPD_HEIGHT, false);
    delay(100);

    float bme280_Altitude = bme.readAltitude(SEALEVELPRESSURE_HPA);

    Blynk.virtualWrite(V0, pressure_event.pressure);
    Blynk.virtualWrite(V1, temp_event.temperature);
    Blynk.virtualWrite(V2, humidity_event.relative_humidity );
    Blynk.virtualWrite(V3, bme280_Altitude);

}


void setup()
{

    Serial.begin(115200);
    // while ( !Serial ) delay(10);


    pinMode(A7682_PWR, OUTPUT);
    // A7682 Power off
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


    delay(200);
    boardInit();
    delay(200);
    LilyGo_logo();
    display->setRotation(3);

    SerialAT.setPins(A7682_TXD, A7682_RXD);
    SerialAT.begin(115200, SERIAL_8N1);
    DBG("Wait...");
    delay(3000);


    // Restart takes quite some time
    // To skip it, call init() instead of restart()
    Serial.println("Initializing modem...");
    if (!modem.init()) {
        Serial.println("Failed to restart modem, attempting to continue without restarting");
    }

    String name = modem.getModemName();
    delay(500);
    Serial.println("Modem Name: " + name);

    Serial.println("...");

    // while (1) {
    //     while (SerialMon.available()) {
    //         SerialAT.write(SerialMon.read());
    //     }
    //     while (SerialAT.available()) {
    //         SerialMon.write(SerialAT.read());
    //     }
    // }


    Blynk.begin(auth, modem, apn, user, pass);
    // Setup a function to be called every second
    timer.setInterval(2000L, sendSensor);

    Serial.println("setup doen");
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



    Blynk.run();
    timer.run();
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
    //enableBacklight(true); //ON backlight
    enableBacklight(false); //OFF  backlight

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
    setupSensor();
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

void loopSensor()
{
    display->setCursor(0, 25);
    // display->print("loading...");
    // display->updateWindow(0, 0, GxEPD_WIDTH, GxEPD_HEIGHT, false);
    //  if (millis() - last > 3000) {
    // return;


    sensors_event_t temp_event, pressure_event;
#ifdef USING_BMP280
    bmp.getTemperatureSensor()->getEvent(&temp_event);
    bmp.getPressureSensor()->getEvent(&pressure_event);
#else
    sensors_event_t humidity_event;
    bme.getTemperatureSensor()->getEvent(&temp_event);
    bme.getPressureSensor()->getEvent(&pressure_event);
    bme.getHumiditySensor()->getEvent(&humidity_event);

    SerialMon.print("Humidity = ");
    SerialMon.print(humidity_event.relative_humidity);
    SerialMon.println(" %");
#endif
    SerialMon.print(F("Temperature = "));
    SerialMon.print(temp_event.temperature);
    SerialMon.println(" *C");

    SerialMon.print(F("Pressure = "));
    SerialMon.print(pressure_event.pressure);
    SerialMon.println(" hPa");

    SerialMon.println();

    display->fillRect(0, 0, GxEPD_WIDTH, GxEPD_HEIGHT, GxEPD_WHITE);

    display->setCursor(0, 25);
    display->setFont(&FreeMonoBold12pt7b);
    display->println("[Temperature]");

    display->setFont(&FreeMonoBold18pt7b);
    display->setCursor(30, 80);
    display->print(temp_event.temperature);

    display->setFont(&FreeMonoBold12pt7b);
    display->setCursor(display->getCursorX(), display->getCursorY() - 20);
    display->println("*C");

    display->setCursor(0, 120);
    display->println("[Pressure]");

    display->setFont(&FreeMonoBold18pt7b);
    display->setCursor(30, 175);
    display->print(pressure_event.pressure);

    display->setFont(&FreeMonoBold12pt7b);
    display->setCursor(display->getCursorX(), display->getCursorY() - 20);
    display->println("%");

    display->updateWindow(0, 0, GxEPD_WIDTH, GxEPD_HEIGHT, false);
    delay(100);

    // last = millis();
    // }
}