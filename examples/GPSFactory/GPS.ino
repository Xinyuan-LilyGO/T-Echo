
#include "utilities.h"
#include <SPI.h>
#include <Wire.h>

#define GNSS_DISABLE_NMEA_OUTPUT "$PCAS03,0,0,0,0,0,0,0,0,0,0,,,0,0*02\r\n"
#define GNSS_GET_VERSION         "$PCAS06,0*1B\r\n"

bool waitResponse(String &data, String rsp, uint32_t timeout)
{
    uint32_t startMillis = millis();
    do {
        while (SerialGPS.available() > 0) {
            int8_t ch = SerialGPS.read();
            data += static_cast<char>(ch);
            if (rsp.length() && data.endsWith(rsp)) {
                return true;
            }
        }
    } while (millis() - startMillis < 1000);
    return false;
}

String                  gpsVersion = "";

bool checkOutput()
{
    uint32_t startTimeout = millis() + 500;
    while (SerialGPS.available()) {
        int ch = SerialGPS.read();
        Serial.write(ch);
        if (millis() > startTimeout) {
            SerialGPS.flush();
            Serial.println("Wait L76K stop output timeout!");
            return true;
        }
    };
    SerialGPS.flush();
    return false;
}

bool gnss_probe()
{
    uint8_t retry = 5;

    while (retry--) {

        SerialGPS.write(GNSS_DISABLE_NMEA_OUTPUT);
        delay(5);
        if (checkOutput()) {
            Serial.println("GPS OUT PUT NOT DISABLE .");
            delay(500);
            continue;
        }
        delay(200);
        SerialGPS.write(GNSS_GET_VERSION);
        if (waitResponse(gpsVersion, "$GPTXT,01,01,02", 1000)) {
            Serial.println("L76K GNSS init succeeded, using L76K GNSS Module\n");
            return true;
        }
        delay(500);
    }
    return false;
}

bool find_gps = false;

void setup()
{
    Serial.begin(115200);

    while (!Serial);

    SerialMon.begin(MONITOR_SPEED);

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

    delay(10);

    pinMode(Gps_Reset_Pin, OUTPUT);
    digitalWrite(Gps_Reset_Pin, HIGH); delay(10);
    digitalWrite(Gps_Reset_Pin, LOW); delay(10);
    digitalWrite(Gps_Reset_Pin, HIGH);

    find_gps = gnss_probe();
    if (!find_gps) {
        Serial.println("GPS not found...");
        return;
    }

    // Initialize the L76K Chip, use GPS + GLONASS
    SerialGPS.write("$PCAS04,5*1C\r\n");
    delay(250);
    // only ask for RMC and GGA
    SerialGPS.write("$PCAS03,1,0,0,0,1,0,0,0,0,0,,,0,0*02\r\n");
    delay(250);
    // Switch to Vehicle Mode, since SoftRF enables Aviation < 2g
    SerialGPS.write("$PCAS11,3*1E\r\n");
}


void loop()
{
    if (!find_gps) {
        Serial.println("GPS not found...");
        delay(1000);
    }
    while (SerialGPS.available()) {
        Serial.write(SerialGPS.read());
    }
    while (Serial.available()) {
        SerialGPS.write(Serial.read());
    }
}

