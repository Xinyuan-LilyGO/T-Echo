/***************************************************************************
* Example sketch for the ICM20948_WE library
*
* This sketch shows how to set the sleep mode for the ICM20948 and its magnetometer.
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

  if(!myIMU.initMagnetometer()){
    Serial.println("Magnetometer does not respond");
  }
  else{
    Serial.println("Magnetometer is connected");
  }

  myIMU.setAccRange(ICM20948_ACC_RANGE_2G);
  myIMU.setAccDLPF(ICM20948_DLPF_6);    
  myIMU.setGyrDLPF(ICM20948_DLPF_6);  
  myIMU.setTempDLPF(ICM20948_DLPF_6);
  myIMU.setMagOpMode(AK09916_CONT_MODE_20HZ);
}

void loop() {
  xyzFloat gValue;
  xyzFloat gyr;
  xyzFloat magValue;
  myIMU.readSensor();
  myIMU.getGValues(&gValue);
  myIMU.getGyrValues(&gyr);
  myIMU.getMagValues(&magValue);
  float temp = myIMU.getTemperature();
  float resultantG = myIMU.getResultantG(&gValue);

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

 /*  Since the ICM20948 controls the AK99916 it is important to first set the
  *  magnetometer to sleep mode and then the ICM20948. For the wake up 
  *  procedure it is the other way round. 
 */
  myIMU.setMagOpMode(AK09916_PWR_DOWN); // set the magnetometer to sleep mode
  delay(200);
  myIMU.sleep(true);  // set the ICM20948 to sleep mode
  delay(10000);
  myIMU.sleep(false);  // wake up the ICM20948
  myIMU.setMagOpMode(AK09916_CONT_MODE_20HZ); // wake up the magnetometer
  delay(200); // give it a bit of time
}
