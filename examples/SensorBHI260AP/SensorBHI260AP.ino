/**
 * @file      SensorBHI260AP.ino
 * @author    Lewis He (lewishe@outlook.com)
 * @license   MIT
 * @copyright Copyright (c) 2025  ShenZhen XinYuan Electronic Technology Co., Ltd
 * @date      2025-04-08
 *
 */

#include <Wire.h>
#include <SPI.h>
#include <Arduino.h>
#include <SensorBHI260AP.hpp>
#include <bosch/BoschSensorDataHelper.hpp>

#define BHI260_SDA  26
#define BHI260_SCL  27

SensorBHI260AP bhy;

/*
* Define the USING_DATA_HELPER use of data assistants.
* No callback function will be used. Data can be obtained directly through
* the data assistant. Note that this method is not a thread-safe function.
* Please pay attention to protecting data access security.
* */
#define USING_DATA_HELPER

#ifdef USING_DATA_HELPER
SensorXYZ accel(SensorBHI260AP::ACCEL_PASSTHROUGH, bhy);
SensorXYZ gyro(SensorBHI260AP::GYRO_PASSTHROUGH, bhy);
#endif

// The firmware runs in RAM and will be lost if the power is off. The firmware will be loaded from RAM each time it is run.
#define BOSCH_APP30_SHUTTLE_BHI260_FW
#include <BoschFirmware.h>

// Force update of current firmware, whether it exists or not.
// Only works when external SPI Flash is connected to BHI260.
// After uploading firmware once, you can change this to false to speed up boot time.
bool force_update_flash_firmware = true;


#ifndef USING_DATA_HELPER
void xyz_process_callback(uint8_t sensor_id, uint8_t *data_ptr, uint32_t len, uint64_t *timestamp, void *user_data)
{
    struct bhy2_data_xyz data;
    float scaling_factor = bhy.getScaling(sensor_id);
    bhy2_parse_xyz(data_ptr, &data);
    Serial.print(bhy.getSensorName(sensor_id));
    Serial.print(" ");
    Serial.print("x: ");
    Serial.print(data.x * scaling_factor);
    Serial.print(", y: ");
    Serial.print(data.y * scaling_factor);
    Serial.print(", z: ");
    Serial.print(data.z * scaling_factor);
    Serial.println(";");
}
#endif

void setup()
{
    Serial.begin(115200);
    while (!Serial);

    Serial.println("Initializing Sensors...");

    // Set the firmware array address and firmware size
    bhy.setFirmware(bosch_firmware_image, bosch_firmware_size);

    // Using I2C interface
    if (!bhy.begin(Wire, BHI260AP_SLAVE_ADDRESS_L, BHI260_SDA, BHI260_SCL)) {
        Serial.print("Failed to initialize sensor - error code:");
        Serial.println(bhy.getError());
        while (1) {
            delay(1000);
        }
    }

    Serial.println("Initializing the sensor successfully!");

    // Output all sensors info to Serial
    BoschSensorInfo info = bhy.getSensorInfo();

    info.printInfo(Serial);

    float sample_rate = 100.0;      /* Read out data measured at 100Hz */
    uint32_t report_latency_ms = 0; /* Report immediately */

#ifdef USING_DATA_HELPER
    // Enable acceleration
    accel.enable(sample_rate, report_latency_ms);
    // Enable gyroscope
    gyro.enable(sample_rate, report_latency_ms);
#else
    // Enable acceleration
    bhy.configure(SensorBHI260AP::ACCEL_PASSTHROUGH, sample_rate, report_latency_ms);
    // Enable gyroscope
    bhy.configure(SensorBHI260AP::GYRO_PASSTHROUGH, sample_rate, report_latency_ms);
    // Set the acceleration sensor result callback function
    bhy.onResultEvent(SensorBHI260AP::ACCEL_PASSTHROUGH, xyz_process_callback);
    // Set the gyroscope sensor result callback function
    bhy.onResultEvent(SensorBHI260AP::GYRO_PASSTHROUGH, xyz_process_callback);
#endif

}


void loop()
{
    // Not interrupt pin connect , use polling method
    bhy.update();

#ifdef USING_DATA_HELPER
    if (accel.hasUpdated() && gyro.hasUpdated()) {
        uint32_t s;
        uint32_t ns;
        accel.getLastTime(s, ns);

#ifdef PLATFORM_HAS_PRINTF
        Serial.printf("[T: %" PRIu32 ".%09" PRIu32 "] AX:%+7.2f AY:%+7.2f AZ:%+7.2f GX:%+7.2f GY:%+7.2f GZ:%+7.2f \n",
                      s, ns, accel.getX(), accel.getY(), accel.getZ(),
                      gyro.getX(), gyro.getY(), gyro.getZ());

#else
        Serial.print("[T: ");
        Serial.print(s);
        Serial.print(".");
        Serial.print(ns);
        Serial.print("] AX:");
        Serial.print(accel.getX(), 2);
        Serial.print(" AY:");
        Serial.print(accel.getY(), 2);
        Serial.print(" AZ:");
        Serial.print(accel.getZ(), 2);
        Serial.print(" GX:");
        Serial.print(gyro.getX(), 2);
        Serial.print(" GY:");
        Serial.print(gyro.getY(), 2);
        Serial.print(" GZ:");
        Serial.print(gyro.getZ(), 2);
        Serial.println();
#endif

    }
#endif  /*USING_DATA_HELPER*/
    delay(50);
}



