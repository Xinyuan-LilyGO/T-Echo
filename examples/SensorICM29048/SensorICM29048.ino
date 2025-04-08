/**
 * @file      SensorICM29048.ino
 * @author    Lewis He (lewishe@outlook.com)
 * @license   MIT
 * @copyright Copyright (c) 2025  ShenZhen XinYuan Electronic Technology Co., Ltd
 * @date      2025-04-08
 *
 */
#include <Wire.h>
#include <ICM20948_WE.h>

#define ICM29048_SDA  26
#define ICM29048_SCL  27

#define ICM20948_ADDR 0x68
ICM20948_WE imu = ICM20948_WE(ICM20948_ADDR);

void setup()
{
    Serial.begin(115200);
    while (!Serial) {}

    Wire.setPins(ICM29048_SDA, ICM29048_SCL);
    Wire.begin();

    if (!imu.init()) {
        Serial.println("ICM20948 does not respond");
    } else {
        Serial.println("ICM20948 is connected");
    }

    if (!imu.initMagnetometer()) {
        Serial.println("Magnetometer does not respond");
    } else {
        Serial.println("Magnetometer is connected");
    }

    /* Check ICM20948 */
    // byte reg = imu.whoAmI();
    //Serial.println(reg);

    /******************* Basic Settings ******************/

    /*  This is a method to calibrate. You have to determine the minimum and maximum
     *  raw acceleration values of the axes determined in the range +/- 2 g.
     *  You call the function as follows: setAccOffsets(xMin,xMax,yMin,yMax,zMin,zMax);
     *  The parameters are floats.
     *  The calibration changes the slope / ratio of raw acceleration vs g. The zero point is
     *  set as (min + max)/2.
     */
    //imu.setAccOffsets(-16330.0, 16450.0, -16600.0, 16180.0, -16520.0, 16690.0);

    /* The starting point, if you position the ICM20948 flat, is not necessarily 0g/0g/1g for x/y/z.
     * The autoOffset function measures offset. It assumes your ICM20948 is positioned flat with its
     * x,y-plane. The more you deviate from this, the less accurate will be your results.
     * It overwrites the zero points of setAccOffsets, but keeps the correction of the slope.
     * The function also measures the offset of the gyroscope data. The gyroscope offset does not
     * depend on the positioning.
     * This function needs to be called after setAccOffsets but before other settings since it will
     * overwrite settings!
     * You can query the offsets with the functions:
     * xyzFloat getAccOffsets() and xyzFloat getGyrOffsets()
     * You can apply the offsets using:
     * setAccOffsets(xyzFloat yourOffsets) and setGyrOffsets(xyzFloat yourOffsets)
     */
    //  Serial.println("Position your ICM20948 flat and don't move it - calibrating...");
    //  delay(1000);
    //  imu.autoOffsets();
    //  Serial.println("Done!");

    /*  The gyroscope data is not zero, even if you don't move the ICM20948.
     *  To start at zero, you can apply offset values. These are the gyroscope raw values you obtain
     *  using the +/- 250 degrees/s range.
     *  Use either autoOffset or setGyrOffsets, not both.
     */
    //imu.setGyrOffsets(-115.0, 130.0, 105.0);

    /*  ICM20948_ACC_RANGE_2G      2 g   (default)
     *  ICM20948_ACC_RANGE_4G      4 g
     *  ICM20948_ACC_RANGE_8G      8 g
     *  ICM20948_ACC_RANGE_16G    16 g
     */
    imu.setAccRange(ICM20948_ACC_RANGE_2G);

    /*  Choose a level for the Digital Low Pass Filter or switch it off.
     *  ICM20948_DLPF_0, ICM20948_DLPF_2, ...... ICM20948_DLPF_7, ICM20948_DLPF_OFF
     *
     *  IMPORTANT: This needs to be ICM20948_DLPF_7 if DLPF is used in cycle mode!
     *
     *  DLPF       3dB Bandwidth [Hz]      Output Rate [Hz]
     *    0              246.0               1125/(1+ASRD)
     *    1              246.0               1125/(1+ASRD)
     *    2              111.4               1125/(1+ASRD)
     *    3               50.4               1125/(1+ASRD)
     *    4               23.9               1125/(1+ASRD)
     *    5               11.5               1125/(1+ASRD)
     *    6                5.7               1125/(1+ASRD)
     *    7              473.0               1125/(1+ASRD)
     *    OFF           1209.0               4500
     *
     *    ASRD = Accelerometer Sample Rate Divider (0...4095)
     *    You achieve lowest noise using level 6
     */
    imu.setAccDLPF(ICM20948_DLPF_6);

    /*  Acceleration sample rate divider divides the output rate of the accelerometer.
     *  Sample rate = Basic sample rate / (1 + divider)
     *  It can only be applied if the corresponding DLPF is not off!
     *  Divider is a number 0...4095 (different range compared to gyroscope)
     *  If sample rates are set for the accelerometer and the gyroscope, the gyroscope
     *  sample rate has priority.
     */
    //imu.setAccSampleRateDivider(10);

    /*  ICM20948_GYRO_RANGE_250       250 degrees per second (default)
     *  ICM20948_GYRO_RANGE_500       500 degrees per second
     *  ICM20948_GYRO_RANGE_1000     1000 degrees per second
     *  ICM20948_GYRO_RANGE_2000     2000 degrees per second
     */
    //imu.setGyrRange(ICM20948_GYRO_RANGE_250);

    /*  Choose a level for the Digital Low Pass Filter or switch it off.
     *  ICM20948_DLPF_0, ICM20948_DLPF_2, ...... ICM20948_DLPF_7, ICM20948_DLPF_OFF
     *
     *  DLPF       3dB Bandwidth [Hz]      Output Rate [Hz]
     *    0              196.6               1125/(1+GSRD)
     *    1              151.8               1125/(1+GSRD)
     *    2              119.5               1125/(1+GSRD)
     *    3               51.2               1125/(1+GSRD)
     *    4               23.9               1125/(1+GSRD)
     *    5               11.6               1125/(1+GSRD)
     *    6                5.7               1125/(1+GSRD)
     *    7              361.4               1125/(1+GSRD)
     *    OFF          12106.0               9000
     *
     *    GSRD = Gyroscope Sample Rate Divider (0...255)
     *    You achieve lowest noise using level 6
     */
    imu.setGyrDLPF(ICM20948_DLPF_6);

    /*  Gyroscope sample rate divider divides the output rate of the gyroscope.
     *  Sample rate = Basic sample rate / (1 + divider)
     *  It can only be applied if the corresponding DLPF is not OFF!
     *  Divider is a number 0...255
     *  If sample rates are set for the accelerometer and the gyroscope, the gyroscope
     *  sample rate has priority.
     */
    //imu.setGyrSampleRateDivider(10);

    /*  Choose a level for the Digital Low Pass Filter.
     *  ICM20948_DLPF_0, ICM20948_DLPF_2, ...... ICM20948_DLPF_7, ICM20948_DLPF_OFF
     *
     *  DLPF          Bandwidth [Hz]      Output Rate [Hz]
     *    0             7932.0                    9
     *    1              217.9                 1125
     *    2              123.5                 1125
     *    3               65.9                 1125
     *    4               34.1                 1125
     *    5               17.3                 1125
     *    6                8.8                 1125
     *    7             7932.0                    9
     *
     *
     *    GSRD = Gyroscope Sample Rate Divider (0...255)
     *    You achieve lowest noise using level 6
     */
    imu.setTempDLPF(ICM20948_DLPF_6);

    /* You can set the following modes for the magnetometer:
     * AK09916_PWR_DOWN          Power down to save energy
     * AK09916_TRIGGER_MODE      Measurements on request, a measurement is triggered by
     *                           calling setMagOpMode(AK09916_TRIGGER_MODE)
     * AK09916_CONT_MODE_10HZ    Continuous measurements, 10 Hz rate
     * AK09916_CONT_MODE_20HZ    Continuous measurements, 20 Hz rate
     * AK09916_CONT_MODE_50HZ    Continuous measurements, 50 Hz rate
     * AK09916_CONT_MODE_100HZ   Continuous measurements, 100 Hz rate (default)
     */
    imu.setMagOpMode(AK09916_CONT_MODE_20HZ);
    // delay(50); // add a delay of 1000/magRate to avoid first mag value being zero
}

void loop()
{
    xyzFloat gValue;
    xyzFloat gyr;
    xyzFloat magValue;

    imu.readSensor();
    imu.getGValues(&gValue);
    imu.getGyrValues(&gyr);
    imu.getMagValues(&magValue);
    float temp = imu.getTemperature();
    float resultantG = imu.getResultantG(&gValue);

    Serial.println("Acceleration in g (x,y,z):");
    Serial.print(gValue.x);
    Serial.print("   ");
    Serial.print(gValue.y);
    Serial.print("   ");
    Serial.println(gValue.z);
    Serial.print("Resultant g: ");
    Serial.println(resultantG);

    Serial.println("Gyroscope data in degrees/s: ");
    Serial.print(gyr.x);
    Serial.print("   ");
    Serial.print(gyr.y);
    Serial.print("   ");
    Serial.println(gyr.z);

    Serial.println("Magnetometer Data in µTesla: ");
    Serial.print(magValue.x);
    Serial.print("   ");
    Serial.print(magValue.y);
    Serial.print("   ");
    Serial.println(magValue.z);

    Serial.print("Temperature in °C: ");
    Serial.println(temp);

    Serial.println("********************************************");

    delay(1000);
}

