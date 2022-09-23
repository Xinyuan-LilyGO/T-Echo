
#define TINY_GSM_MODEM_SIM7600 //A7608 Compatible with SIM7600 AT instructions
#define TINY_GSM_RX_BUFFER 1024 // Set RX buffer to 1Kb
#define SerialAT Serial1

// Set serial for debug console (to the Serial Monitor, default speed 115200)
#define SerialMon Serial

// See all AT commands, if wanted
#define DUMP_AT_COMMANDS


#define UART_BAUD   115200
#define SerialAT Serial1

#define VBAT_MV_PER_LSB   (0.73242188F)   // 3.0V ADC range and 12-bit ADC resolution = 3000mV/4096
#define VBAT_DIVIDER      (0.5F)
#define VBAT_DIVIDER_COMP (2.0F)
#define REAL_VBAT_MV_PER_LSB (VBAT_DIVIDER_COMP * VBAT_MV_PER_LSB)

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
#include <Adafruit_BME280.h>
#include <CayenneMQTTGSM.h>


#define SEALEVELPRESSURE_HPA (1013.25)
#define TEMPERATURE_VIRTUAL_CHANNEL         1
#define BAROMETER_VIRTUAL_CHANNEL           2
#define ALTITUDE_VIRTUAL_CHANNEL            3
#define BATTERY_VIRTUAL_CHANNEL             4
#define LIGHTSENSOR_VIRTUAL_CHANNEL         5

// GSM connection info.
const char apn[]      = "YourAPN";
// const char apn[]      = "CMNET";
char gprsLogin[] = ""; // GPRS username. Leave empty if it is not needed.
char gprsPassword[] = ""; // GPRS password. Leave empty if it is not needed.
char pin[] = ""; // SIM pin number. Leave empty if it is not needed.

// Cayenne authentication info. This should be obtained from the Cayenne Dashboard.
char username[] = "Cayenne username";
char password[] = "Cayenne password";
char clientID[] = "Cayenne clientID";

bool bmpSensorDetected = true;
float mv_per_lsb = 3600.0F / 1024.0F; // 10-bit ADC with 3.6V input range

SPIClass        *dispPort  = nullptr;
SPIClass        *rfPort    = nullptr;
GxIO_Class      *io        = nullptr;
GxEPD_Class     *display   = nullptr;

uint32_t        blinkMillis = 0;
uint8_t rgb = 0;
bool reply = false;
Adafruit_BME280 bme;


void setup()
{

    Serial.begin(115200);
    // while ( !Serial ) delay(10);


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


    delay(200);
    boardInit();
    delay(200);
    LilyGo_logo();
    display->fillScreen(GxEPD_WHITE);
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

    delay(500);
    Serial.println("Waiting for network...");
    if (!modem.waitForNetwork()) {
        delay(10000);
        return;
    }

    if (modem.isNetworkConnected()) {
        Serial.println("Network connected");
    }
    delay(5000);

    // while (1) {
    //     while (SerialMon.available()) {
    //         SerialAT.write(SerialMon.read());
    //     }
    //     while (SerialAT.available()) {
    //         SerialMon.write(SerialAT.read());
    //     }
    // }


    Serial.println("...");
    Cayenne.begin(username, password, clientID, SerialAT, apn, gprsLogin, gprsPassword, pin);

    Serial.println("setup doen");
}

void loop()
{
    if (millis() - blinkMillis > 2000) {
        loopSensor();
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

    // while (SerialMon.available()) {
    //     SerialAT.write(SerialMon.read());
    // }
    // while (SerialAT.available()) {
    //     SerialMon.write(SerialAT.read());
    // }

    Cayenne.loop();
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


// Default function for processing actuator commands from the Cayenne Dashboard.
// You can also use functions for specific channels, e.g CAYENNE_IN(1) for channel 1 commands.
CAYENNE_IN_DEFAULT()
{
    CAYENNE_LOG("Channel %u, value %s", request.channel, getValue.asString());
    //Process message here. If there is an error set an error message using getValue.setError(), e.g getValue.setError("Error message");
}

CAYENNE_IN(1)
{
    CAYENNE_LOG("Channel %u, value %s", request.channel, getValue.asString());
}


// This function is called at intervals to send temperature sensor data to Cayenne.
CAYENNE_OUT(TEMPERATURE_VIRTUAL_CHANNEL)
{

    if (bmpSensorDetected) {
        float temperature = bme.readTemperature();

        Serial.print("Temperature = ");
        Serial.print(temperature);
        Serial.println(" *C");

        Cayenne.celsiusWrite(TEMPERATURE_VIRTUAL_CHANNEL, temperature);
    }
}

// This function is called at intervals to send barometer sensor data to Cayenne.
CAYENNE_OUT(BAROMETER_VIRTUAL_CHANNEL)
{
    if (bmpSensorDetected) {
        float pressure = bme.readPressure() / 1000;

        Serial.print("Pressure = ");
        Serial.print(pressure );
        Serial.println(" hPa");

        Cayenne.hectoPascalWrite(BAROMETER_VIRTUAL_CHANNEL, pressure);
    }
}


CAYENNE_OUT(ALTITUDE_VIRTUAL_CHANNEL)
{
    if (bmpSensorDetected) {
        float altitude = bme.readAltitude(SEALEVELPRESSURE_HPA);

        Serial.print("Altitude = ");
        Serial.print(altitude);
        Serial.println(" meters");

        Cayenne.virtualWrite(ALTITUDE_VIRTUAL_CHANNEL, altitude, "meters", UNIT_METER);

    }
}





float readVBAT(uint8_t pin)
{
    float raw;

    // Set the analog reference to 3.0V (default = 3.6V)
    analogReference(AR_INTERNAL_3_0);

    // Set the resolution to 12-bit (0..4095)
    analogReadResolution(12); // Can be 8, 10, 12 or 14

    // Let the ADC settle
    delay(1);

    // Get the raw 12-bit, 0..3000mV ADC value
    raw = analogRead(pin);

    // Set the ADC back to the default settings
    analogReference(AR_DEFAULT);
    analogReadResolution(10);

    // Convert the raw value to compensated mv, taking the resistor-
    // divider into account (providing the actual LIPO voltage)
    // ADC range is 0..3000mV and resolution is 12-bit (0..4095)
    return raw * REAL_VBAT_MV_PER_LSB;
}


CAYENNE_OUT(BATTERY_VIRTUAL_CHANNEL)
{
    float mv = readVBAT(Adc_Pin) ;
    Serial.printf("batter : %f\n", mv);
    Cayenne.virtualWrite(BATTERY_VIRTUAL_CHANNEL, mv, TYPE_VOLTAGE, UNIT_MILLIVOLTS);

}
