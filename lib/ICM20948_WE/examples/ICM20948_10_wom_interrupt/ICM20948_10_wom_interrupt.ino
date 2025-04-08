/***************************************************************************
* Example sketch for the ICM20948_WE library
*
* This sketch shows how to use the wake-on-motion interrupt. 
* Interestingly, the ICM20948 does not seem to wake up from power down mode even 
* when the wake-on-motion conditions are met. To me the name "wake on motion" is 
* a bit misleading. It's just an acceleration controlled interrupt.
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

const int intPin = 2;
volatile bool motion = false;

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

  /*  This is a method to calibrate. You have to determine the minimum and maximum 
   *  raw acceleration values of the axes determined in the range +/- 2 g. 
   *  You call the function as follows: setAccOffsets(xMin,xMax,yMin,yMax,zMin,zMax);
   *  The parameters are floats. 
   *  The calibration changes the slope / ratio of raw acceleration vs g. The zero point is 
   *  set as (min + max)/2.
   */
  //myIMU.setAccOffsets(-16330.0, 16450.0, -16600.0, 16180.0, -16520.0, 16690.0);
    
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
  Serial.println("Position your ICM20948 flat and don't move it - calibrating...");
  delay(1000);
  myIMU.autoOffsets();
  Serial.println("Done!"); 
  
  /* enables or disables the acceleration sensor, default: enabled */
  // myIMU.enableAcc(true);

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
  myIMU.setAccDLPF(ICM20948_DLPF_6);    
  
  /*  Acceleration sample rate divider divides the output rate of the accelerometer.
   *  Sample rate = Basic sample rate / (1 + divider) 
   *  It can only be applied if the corresponding DLPF is not off!
   *  Divider is a number 0...4095 (different range compared to gyroscope)
   *  If sample rates are set for the accelerometer and the gyroscope, the gyroscope
   *  sample rate has priority.
   */
  myIMU.setAccSampleRateDivider(10);
  
  /*  Set the interrupt pin:
   *  ICM20948_ACT_LOW  = active-low
   *  ICM20948_ACT_HIGH = active-high (default) 
   */
   //myIMU.setIntPinPolarity(ICM20948_ACT_LOW);

   /*  If latch is enabled the interrupt pin level is held until the interrupt status 
   *  is cleared. If latch is disabled the interrupt pulse is ~50µs (default).
   */
  myIMU.enableIntLatch(true);

  /*  The interrupts ICM20948_FSYNC_INT, ICM20948_WOM_INT and ICM20948_DMP_INT can be 
   *  cleared by any read or will only be cleared if the interrupt status register is 
   *  read (default).
   */
//  myIMU.enableClearIntByAnyRead(true);

  /*  Set the FSync interrupt pin:
   *  ICM20948_ACT_LOW  = active-low
   *  ICM20948_ACT_HIGH = active-high (default) 
   */
   //myIMU.setFSyncIntPinPolarity(ICM20948_ACT_LOW);

  /* The following interrupts can be enabled or disabled:
   *  ICM20948_FSYNC_INT        FSYNC pin interrupt, can't propagate to the INT pin  
   *  ICM20948_WOM_INT          Wake on motion
   *  ICM20948_DATA_READY_INT   New data available
   *  ICM20948_FIFO_OVF_INT     FIFO overflow
   */
  myIMU.enableInterrupt(ICM20948_WOM_INT);
  //myIMU.disableInterrupt(ICM20948_WOM_INT);

  /* setWakeOnMotionThreshold sets the limit (first argument) for a wake on 
   * motion interrupt and defines the WOM mode (second argument). The limit 
   * is a number between 1 and 255 with 1 = 4 mg (milli-g) and 255 = 1020 mg. 
   * You can choose from two modes:
   * ICM20948_WOM_COMP_DISABLE = compare the sample with the initial sample (default). 
   * ICM20948_WOM_COMP_ENABLE = compare the sampe with the previous sample.
   */
  myIMU.setWakeOnMotionThreshold(128, ICM20948_WOM_COMP_DISABLE);
  attachInterrupt(digitalPinToInterrupt(intPin), motionISR, RISING);
  Serial.println("Turn your ICM-20948 and see what happens...");
  myIMU.readAndClearInterrupts();
  motion = false;
 }

void loop() {
  xyzFloat gValue;

  if((millis()%1000) == 0){
    myIMU.readSensor();
    myIMU.getGValues(&gValue);
    
    Serial.print(gValue.x);
    Serial.print("   ");
    Serial.print(gValue.y);
    Serial.print("   ");
    Serial.println(gValue.z);
  }
  if(motion){
    byte source = myIMU.readAndClearInterrupts();
    Serial.println("Interrupt!");
    if(myIMU.checkInterrupt(source, ICM20948_WOM_INT)){
      Serial.println("Interrupt Type: Motion");
      delay(1000);
    }
    motion = false;
    // if additional interrupts have occured in the meantime:
    myIMU.readAndClearInterrupts(); 
  }
}

void motionISR() {
  motion = true;
}
