/***************************************************************************
* Example sketch for the ICM20948_WE library
*
* This sketch shows how to use the cycle mode. Enable/Disable the gyrosocope and
* accelerometer and play with the sample rates to see how the update frequency of the
* displayed measured values change. 
* 
* Further information can be found on:
*
* https://wolles-elektronikkiste.de/icm-20948-9-achsensensor-teil-i (German)
* https://wolles-elektronikkiste.de/en/icm-20948-9-axis-sensor-part-i (English)
* 
***************************************************************************/

#include <Wire.h>
#include <ICM20948_WE.h>
#define ICM20948_ADDR 0x68

/* There are several ways to create your ICM20948 object:
 * ICM20948_WE myIMU = ICM20948_WE()              -> uses Wire / I2C Address = 0x68
 * ICM20948_WE myIMU = ICM20948_WE(ICM20948_ADDR) -> uses Wire / ICM20948_ADDR
 * ICM20948_WE myIMU = ICM20948_WE(&wire2)        -> uses the TwoWire object wire2 / ICM20948_ADDR
 * ICM20948_WE myIMU = ICM20948_WE(&wire2, ICM20948_ADDR) -> all together
 * ICM20948_WE myIMU = ICM20948_WE(CS_PIN, spi);  -> uses SPI, spi is just a flag, see SPI example
 * ICM20948_WE myIMU = ICM20948_WE(&SPI, CS_PIN, spi);  -> uses SPI / passes the SPI object, spi is just a flag, see SPI example
 */
ICM20948_WE myIMU = ICM20948_WE(ICM20948_ADDR);

void setup() {
  //delay(2000); // maybe needed for some MCUs, in particular for startup after power off
  Wire.begin();
  Serial.begin(115200);
  while(!Serial) {}
  
  if(!myIMU.init()){
    Serial.println("ICM20948 does not respond");
  }
  else{
    Serial.println("ICM20948 is connected");
  }

  if (!myIMU.initMagnetometer()) {
    Serial.println("Magnetometer does not respond");
  }
  else {
    Serial.println("Magnetometer is connected");
  }

  /*  This is a method to calibrate. You have to determine the minimum and maximum 
   *  raw acceleration values of the axes determined in the range +/- 2 g. 
   *  You call the function as follows: setAccOffsets(xMin,xMax,yMin,yMax,zMin,zMax);
   *  The parameters are floats. 
   *  The calibration changes the slope / ratio of raw acceleration vs g. The zero point is 
   *  set as (min + max)/2.
   */
//  myIMU.setAccOffsets(-16330.0, 16450.0, -16600.0, 16180.0, -16640.0, 16560.0);
    
  /*  The starting point, if you position the ICM20948 flat, is not necessarily 0g/0g/1g for x/y/z. 
   *  The autoOffset function measures offset. It assumes your ICM20948 is positioned flat with its 
   *  x,y-plane. The more you deviate from this, the less accurate will be your results.
   *  It overwrites the zero points of setAccOffsets, but keeps the correction of the slope.
   *  The function also measures the offset of the gyroscope data. The gyroscope offset does not   
   *  depend on the positioning.
   *  This function needs to be called after setAccOffsets but before other settings since it will 
   *  overwrite settings!
   *  You can query the offsets with the functions:
   *  xyzFloat getAccOffsets() and xyzFloat getGyrOffsets()
   *  You can apply the offsets using:
   *  setAccOffsets(xyzFloat yourOffsets) and setGyrOffsets(xyzFloat yourOffsets)
   */
//  Serial.println("Position your ICM20948 flat and don't move it - calibrating...");
//  delay(1000);
//  myIMU.autoOffsets();
//  Serial.println("Done!"); 
  
  /*  The gyroscope data is not zero, even if you don't move the ICM20948. 
   *  To start at zero, you can apply offset values. These are the gyroscope raw values you obtain
   *  using the +/- 250 degrees/s range. 
   *  Use either autoOffset or setGyrOffsets, not both.
   */
//  myIMU.setGyrOffsets(-115.0, 130.0, 105.0);
  
  
  /* enables or disables the acceleration sensor, default: enabled */
  //myIMU.enableAcc(false);

  /*  ICM20948_ACC_RANGE_2G      2 g   (default)
   *  ICM20948_ACC_RANGE_4G      4 g
   *  ICM20948_ACC_RANGE_8G      8 g   
   *  ICM20948_ACC_RANGE_16G    16 g
   */
  myIMU.setAccRange(ICM20948_ACC_RANGE_2G);
  
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
  myIMU.setAccDLPF(ICM20948_DLPF_7);    
  
  /*  Acceleration sample rate divider divides the output rate of the accelerometer.
   *  Sample rate = Basic sample rate / (1 + divider) 
   *  It can only be applied if the corresponding DLPF is not off!
   *  Divider is a number 0...4095 (different range compared to gyroscope)
   *  If sample rates are set for the accelerometer and the gyroscope, the gyroscope
   *  sample rate has priority.
   */
  myIMU.setAccSampleRateDivider(2048);
  
  /* enables or disables the gyroscope sensor, default: enabled */
  //myIMU.enableGyr(false);

  /*  ICM20948_GYRO_RANGE_250       250 degrees per second (default)
   *  ICM20948_GYRO_RANGE_500       500 degrees per second
   *  ICM20948_GYRO_RANGE_1000     1000 degrees per second
   *  ICM20948_GYRO_RANGE_2000     2000 degrees per second
   */
 // myIMU.setGyrRange(ICM20948_GYRO_RANGE_250);
  
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
  myIMU.setGyrDLPF(ICM20948_DLPF_6);  
  
  /*  Gyroscope sample rate divider divides the output rate of the gyroscope.
   *  Sample rate = Basic sample rate / (1 + divider) 
   *  It can only be applied if the corresponding DLPF is not OFF!
   *  Divider is a number 0...255
   *  If sample rates are set for the accelerometer and the gyroscope, the gyroscope
   *  sample rate has priority.
   */
  myIMU.setGyrSampleRateDivider(255);

  /* In cycle mode the sensors wake up in the frequency of sample rate. You can define 
   * which sensors shall work in cycle mode. 
   * 
   *  ICM20948_NO_CYCLE          
   *  ICM20948_GYR_CYCLE           
   *  ICM20948_ACC_CYCLE           
   *  ICM20948_ACC_GYR_CYCLE       
   *  ICM20948_ACC_GYR_I2C_MST_CYCLE 
   */
  myIMU.enableCycle(ICM20948_ACC_GYR_I2C_MST_CYCLE);
  
  /* The Low Power Mode does not work if DLPF is enabled */
  //myIMU.enableLowPower(true);
    
  /* In cycle mode you need to set the number of measured values for the gyroscope 
   * to be averaged. You can select 1,2,4,8,16,32,64 or 128: 
   * ICM20948_GYR_AVG_1, ICM20948_GYR_AVG_2, ..... , ICM20948_GYR_AVG_128 
   * The on-time for measurements increases with the number of samples to be averaged:
   * 1 -> 1.15 ms, 2 -> 1.59 ms, .... , 128 -> 57.59 ms (see data sheet)
   */
  myIMU.setGyrAverageInCycleMode(ICM20948_GYR_AVG_1);

  /* In cycle mode you need to set the number of measured values for the accelerometer 
   * to be averaged. You can select 4,8,16,32: 
   * ICM20948_ACC_AVG_4, ICM20948_ACC_AVG_8, ICM20948_ACC_AVG_16, ICM20948_ACC_AVG_32 
   * The on-time for measurements increases with the number of samples to be averaged:
   * 4 -> 1.488 ms, 8 -> 2.377 ms, 16 -> 4.154 ms , 32 -> 7.71 ms (see data sheet)
   * If DLPF is OFF, then with ICM20948_ACC_AVG_4 only one sample is taken!
   */
  myIMU.setAccAverageInCycleMode(ICM20948_ACC_AVG_4);

  /* setI2CMstSampleRate sets the rate of the devices controlled by the I2C master,
   * i.e. the magnetometer. It is not the internal sample rate of the magnetometer, but
   * the output rate of the I2C master. Allowed values are x = 0...15. The sample rate is
   * 1.1 kHz / (2^x). Example: x = 13 => Sample rate = 1.1 kHz / 8192 = ~0.1343 Hz, 
   * or: data output every ~7.45 seconds.
   */
  myIMU.setI2CMstSampleRate(13); 
}

void loop() {
  xyzFloat gValue; 
  xyzFloat gyrValue; 
  xyzFloat magValue; 
  myIMU.readSensor();
  myIMU.getGValues(&gValue);
  myIMU.getGyrValues(&gyrValue);
  myIMU.getMagValues(&magValue); 
      
  Serial.println("g-values (x,y,z):");
  Serial.print(gValue.x);
  Serial.print("   ");
  Serial.print(gValue.y);
  Serial.print("   ");
  Serial.println(gValue.z);

  Serial.println("Gyroscope (x,y,z):");
  Serial.print(gyrValue.x);
  Serial.print("   ");
  Serial.print(gyrValue.y);
  Serial.print("   ");
  Serial.println(gyrValue.z);

  Serial.println("Magnetometer Data in µTesla: ");
  Serial.print(magValue.x);
  Serial.print("   ");
  Serial.print(magValue.y);
  Serial.print("   ");
  Serial.println(magValue.z);

  Serial.println();

  delay(500);
}
