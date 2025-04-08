/********************************************************************
* This is a library for the 9-axis gyroscope, accelerometer and magnetometer ICM20948.
*
* You'll find an example which should enable you to use the library. 
*
* You are free to use it, change it or build on it. In case you like 
* it, it would be cool if you give it a star.
* 
* If you find bugs, please inform me!
* 
* Written by Wolfgang (Wolle) Ewald
*
* Further information can be found on:
*
* https://wolles-elektronikkiste.de/icm-20948-9-achsensensor-teil-i (German)
* https://wolles-elektronikkiste.de/en/icm-20948-9-axis-sensor-part-i (English)
*
*********************************************************************/

#include "ICM20948_WE.h"

/************ Basic Settings ************/ 

bool ICM20948_WE::init(){
    if(useSPI){
        pinMode(csPin, OUTPUT);
        digitalWrite(csPin, HIGH);
#if defined(ESP32)
        if(spiPinsChanged){
            _spi->begin(sclPin, misoPin, mosiPin, csPin);
        }
        else{
            _spi->begin();
        }
#else
    _spi->begin();
#endif
        mySPISettings = SPISettings(7000000, MSBFIRST, SPI_MODE0);      
    }   
    currentBank = 0;
    
    reset_ICM20948();
    
    uint8_t tries = 0;
    while(( whoAmI() != ICM20948_WHO_AM_I_CONTENT) && (tries < 10)){
        reset_ICM20948();
        delay(300);
        tries++;        
    }
    if(tries == 10) return false;
    
    accOffsetVal.x = 0.0;
    accOffsetVal.y = 0.0;
    accOffsetVal.z = 0.0;
    accCorrFactor.x = 1.0;
    accCorrFactor.y = 1.0;
    accCorrFactor.z = 1.0;
    accRangeFactor = 1.0;
    gyrOffsetVal.x = 0.0;
    gyrOffsetVal.y = 0.0;
    gyrOffsetVal.z = 0.0;
    gyrRangeFactor = 1.0;
    fifoType = ICM20948_FIFO_ACC;
        
    sleep(false);
    writeRegister8(2, ICM20948_ODR_ALIGN_EN, 1); // aligns ODR 
    
    return true;
}

void ICM20948_WE::autoOffsets(){
    xyzFloat accRawVal, gyrRawVal;
    accOffsetVal.x = 0.0;
    accOffsetVal.y = 0.0;
    accOffsetVal.z = 0.0;
        
    setGyrDLPF(ICM20948_DLPF_6); // lowest noise
    setGyrRange(ICM20948_GYRO_RANGE_250); // highest resolution
    setAccRange(ICM20948_ACC_RANGE_2G);
    setAccDLPF(ICM20948_DLPF_6);
    delay(100);

    for(int i=0; i<10; i++){  // Allow to get stable values
        readSensor();
        delay(10);
    }
    
    for(int i=0; i<50; i++){
        readSensor();
        getAccRawValues(&accRawVal);
        accOffsetVal.x += accRawVal.x;
        accOffsetVal.y += accRawVal.y;
        accOffsetVal.z += accRawVal.z;
        delay(10);
    }
    
    accOffsetVal.x /= 50;
    accOffsetVal.y /= 50;
    accOffsetVal.z /= 50;
    accOffsetVal.z -= 16384.0;
    
    for(int i=0; i<50; i++){
        readSensor();
        getGyrRawValues(&gyrRawVal);
        gyrOffsetVal.x += gyrRawVal.x;
        gyrOffsetVal.y += gyrRawVal.y;
        gyrOffsetVal.z += gyrRawVal.z;
        delay(1);
    }
    
    gyrOffsetVal.x /= 50;
    gyrOffsetVal.y /= 50;
    gyrOffsetVal.z /= 50;
    
}

void ICM20948_WE::setAccOffsets(float xMin, float xMax, float yMin, float yMax, float zMin, float zMax){
    accOffsetVal.x = (xMax + xMin) * 0.5;
    accOffsetVal.y = (yMax + yMin) * 0.5;
    accOffsetVal.z = (zMax + zMin) * 0.5;
    accCorrFactor.x = (xMax + abs(xMin)) / 32768.0;
    accCorrFactor.y = (yMax + abs(yMin)) / 32768.0;
    accCorrFactor.z = (zMax + abs(zMin)) / 32768.0 ;    
}

void ICM20948_WE::setAccOffsets(xyzFloat offset){
    accOffsetVal = offset;
}

xyzFloat ICM20948_WE::getAccOffsets(){
    return accOffsetVal;
}

void ICM20948_WE::setGyrOffsets(float xOffset, float yOffset, float zOffset){
    gyrOffsetVal.x = xOffset;
    gyrOffsetVal.y = yOffset;
    gyrOffsetVal.z = zOffset;
}

void ICM20948_WE::setGyrOffsets(xyzFloat offset){
    gyrOffsetVal = offset;
}

xyzFloat ICM20948_WE::getGyrOffsets(){
    return gyrOffsetVal;
}

uint8_t ICM20948_WE::whoAmI(){
    return readRegister8(0, ICM20948_WHO_AM_I);
}

void ICM20948_WE::enableAcc(bool enAcc){
    regVal = readRegister8(0, ICM20948_PWR_MGMT_2);
    if(enAcc){
        regVal &= ~ICM20948_ACC_EN;
    }
    else{
        regVal |= ICM20948_ACC_EN;
    }
    writeRegister8(0, ICM20948_PWR_MGMT_2, regVal);
}

void ICM20948_WE::setAccRange(ICM20948_accRange accRange){
    regVal = readRegister8(2, ICM20948_ACCEL_CONFIG);
    regVal &= ~(0x06);
    regVal |= (accRange<<1);
    writeRegister8(2, ICM20948_ACCEL_CONFIG, regVal);
    accRangeFactor = 1<<accRange;
}

void ICM20948_WE::setAccDLPF(ICM20948_dlpf dlpf){
    regVal = readRegister8(2, ICM20948_ACCEL_CONFIG);
    if(dlpf==ICM20948_DLPF_OFF){
        regVal &= 0xFE;
        writeRegister8(2, ICM20948_ACCEL_CONFIG, regVal);
        return;
    }
    else{
        regVal |= 0x01;
        regVal &= 0xC7;
        regVal |= (dlpf<<3);
    }
    writeRegister8(2, ICM20948_ACCEL_CONFIG, regVal);
}   

void ICM20948_WE::setAccSampleRateDivider(uint16_t accSplRateDiv){
    writeRegister16(2, ICM20948_ACCEL_SMPLRT_DIV_1, accSplRateDiv);
}

void ICM20948_WE::enableGyr(bool enGyr){
    regVal = readRegister8(0, ICM20948_PWR_MGMT_2);
    if(enGyr){
        regVal &= ~ICM20948_GYR_EN;
    }
    else{
        regVal |= ICM20948_GYR_EN;
    }
    writeRegister8(0, ICM20948_PWR_MGMT_2, regVal);
}

void ICM20948_WE::setGyrRange(ICM20948_gyroRange gyroRange){
    regVal = readRegister8(2, ICM20948_GYRO_CONFIG_1);
    regVal &= ~(0x06);
    regVal |= (gyroRange<<1);
    writeRegister8(2, ICM20948_GYRO_CONFIG_1, regVal);
    gyrRangeFactor = (1<<gyroRange);
}

void ICM20948_WE::setGyrDLPF(ICM20948_dlpf dlpf){
    regVal = readRegister8(2, ICM20948_GYRO_CONFIG_1);
    if(dlpf==ICM20948_DLPF_OFF){
        regVal &= 0xFE;
        writeRegister8(2, ICM20948_GYRO_CONFIG_1, regVal);
        return;
    }
    else{
        regVal |= 0x01;
        regVal &= 0xC7;
        regVal |= (dlpf<<3);
    }
    writeRegister8(2, ICM20948_GYRO_CONFIG_1, regVal);
}   

void ICM20948_WE::setGyrSampleRateDivider(uint8_t gyrSplRateDiv){
    writeRegister8(2, ICM20948_GYRO_SMPLRT_DIV, gyrSplRateDiv);
}

void ICM20948_WE::setTempDLPF(ICM20948_dlpf dlpf){
    writeRegister8(2, ICM20948_TEMP_CONFIG, dlpf);
}

void ICM20948_WE::setI2CMstSampleRate(uint8_t rateExp){
    if(rateExp < 16){
        writeRegister8(3, ICM20948_I2C_MST_ODR_CFG, rateExp);
    }
}

void ICM20948_WE::setSPIClockSpeed(unsigned long clock){
    mySPISettings = SPISettings(clock, MSBFIRST, SPI_MODE0);
}
    
/************* x,y,z results *************/
        
void ICM20948_WE::readSensor(){
    readAllData(buffer);
}

void ICM20948_WE::getAccRawValues(xyzFloat *accRawVal){
    accRawVal->x = static_cast<int16_t>(((buffer[0]) << 8) | buffer[1]) * 1.0;
    accRawVal->y = static_cast<int16_t>(((buffer[2]) << 8) | buffer[3]) * 1.0;
    accRawVal->z = static_cast<int16_t>(((buffer[4]) << 8) | buffer[5]) * 1.0;
}

void ICM20948_WE::getCorrectedAccRawValues(xyzFloat *corrAccRawVal){
    getAccRawValues(&(*corrAccRawVal));   
    correctAccRawValues(&(*corrAccRawVal));
}

void ICM20948_WE::getGValues(xyzFloat *gVal){
    getCorrectedAccRawValues(&(*gVal));
    
    gVal->x = gVal->x * accRangeFactor / 16384.0;
    gVal->y = gVal->y * accRangeFactor / 16384.0;
    gVal->z = gVal->z * accRangeFactor / 16384.0;
}

void ICM20948_WE::getAccRawValuesFromFifo(xyzFloat *accRawVal){
    readICM20948xyzValFromFifo(&(*accRawVal)); 
}

void ICM20948_WE::getCorrectedAccRawValuesFromFifo(xyzFloat *accRawVal){
    getAccRawValuesFromFifo(&(*accRawVal));
    correctAccRawValues(&(*accRawVal));
}

void ICM20948_WE::getGValuesFromFifo(xyzFloat *gVal){
    getCorrectedAccRawValuesFromFifo(&(*gVal));
    
    gVal->x = gVal->x * accRangeFactor / 16384.0;
    gVal->y = gVal->y * accRangeFactor / 16384.0;
    gVal->z = gVal->z * accRangeFactor / 16384.0;
}

float ICM20948_WE::getResultantG(xyzFloat *gVal){
    float resultant = 0.0;
    resultant = sqrt(sq(gVal->x) + sq(gVal->y) + sq(gVal->z));
    
    return resultant;
}

float ICM20948_WE::getTemperature(){
    int16_t rawTemp = static_cast<int16_t>(((buffer[12]) << 8) | buffer[13]);
    float tmp = (rawTemp*1.0 - ICM20948_ROOM_TEMP_OFFSET)/ICM20948_T_SENSITIVITY + 21.0;
    return tmp;
}

void ICM20948_WE::getGyrRawValues(xyzFloat *gyrRawVal){
    gyrRawVal->x = (int16_t)(((buffer[6]) << 8) | buffer[7]) * 1.0;
    gyrRawVal->y = (int16_t)(((buffer[8]) << 8) | buffer[9]) * 1.0;
    gyrRawVal->z = (int16_t)(((buffer[10]) << 8) | buffer[11]) * 1.0;
}

void ICM20948_WE::getCorrectedGyrRawValues(xyzFloat *corrGyrVal){
    getGyrRawValues(&(*corrGyrVal)); 
    correctGyrRawValues(&(*corrGyrVal));
}

void ICM20948_WE::getGyrValues(xyzFloat *gyrVal){
    getCorrectedGyrRawValues(&(*gyrVal));
    
    gyrVal->x = gyrVal->x * gyrRangeFactor * 250.0 / 32768.0;
    gyrVal->y = gyrVal->y * gyrRangeFactor * 250.0 / 32768.0;
    gyrVal->z = gyrVal->z * gyrRangeFactor * 250.0 / 32768.0;
}

void ICM20948_WE::getGyrValuesFromFifo(xyzFloat *gyrVal){
    readICM20948xyzValFromFifo(&(*gyrVal));
    
    correctGyrRawValues(&(*gyrVal));
    gyrVal->x = gyrVal->x * gyrRangeFactor * 250.0 / 32768.0;
    gyrVal->y = gyrVal->y * gyrRangeFactor * 250.0 / 32768.0;
    gyrVal->z = gyrVal->z * gyrRangeFactor * 250.0 / 32768.0;
}

void ICM20948_WE::getMagValues(xyzFloat *mag){
    int16_t x,y,z;
    
    x = static_cast<int16_t>((buffer[15]) << 8) | buffer[14];
    y = static_cast<int16_t>((buffer[17]) << 8) | buffer[16];
    z = static_cast<int16_t>((buffer[19]) << 8) | buffer[18];
    
    mag->x = x * AK09916_MAG_LSB;
    mag->y = y * AK09916_MAG_LSB;
    mag->z = z * AK09916_MAG_LSB;
}

/********* Power, Sleep, Standby *********/ 

void ICM20948_WE::enableCycle(ICM20948_cycle cycle){
    regVal = readRegister8(0, ICM20948_LP_CONFIG);
    regVal &= 0x0F;
    regVal |= cycle;
    
    writeRegister8(0, ICM20948_LP_CONFIG, regVal);
}

void ICM20948_WE::enableLowPower(bool enLP){    // vielleicht besser privat????
    regVal = readRegister8(0, ICM20948_PWR_MGMT_1);
    if(enLP){
        regVal |= ICM20948_LP_EN;
    }
    else{
        regVal &= ~ICM20948_LP_EN;
    }
    writeRegister8(0, ICM20948_PWR_MGMT_1, regVal);
}

void ICM20948_WE::setGyrAverageInCycleMode(ICM20948_gyroAvgLowPower avg){
    writeRegister8(2, ICM20948_GYRO_CONFIG_2, avg);
}

void ICM20948_WE::setAccAverageInCycleMode(ICM20948_accAvgLowPower avg){
    writeRegister8(2, ICM20948_ACCEL_CONFIG_2, avg);
}

void ICM20948_WE::sleep(bool sleep){
    regVal = readRegister8(0, ICM20948_PWR_MGMT_1);
    if(sleep){
        regVal |= ICM20948_SLEEP;
    }
    else{
        regVal &= ~ICM20948_SLEEP;
    }
    writeRegister8(0, ICM20948_PWR_MGMT_1, regVal);
}
        
/******** Angles and Orientation *********/ 
    
void ICM20948_WE::getAngles(xyzFloat *angleVal){
    xyzFloat gVal;
    getGValues(&gVal);
    if(gVal.x > 1.0){
        gVal.x = 1.0;
    }
    else if(gVal.x < -1.0){
        gVal.x = -1.0;
    }
    angleVal->x = (asin(gVal.x)) * 57.296;
    
    if(gVal.y > 1.0){
        gVal.y = 1.0;
    }
    else if(gVal.y < -1.0){
        gVal.y = -1.0;
    }
    angleVal->y = (asin(gVal.y)) * 57.296;
    
    if(gVal.z > 1.0){
        gVal.z = 1.0;
    }
    else if(gVal.z < -1.0){
        gVal.z = -1.0;
    }
    angleVal->z = (asin(gVal.z)) * 57.296;
}

ICM20948_orientation ICM20948_WE::getOrientation(){
    xyzFloat angleVal;
    getAngles(&angleVal);
    ICM20948_orientation orientation = ICM20948_FLAT;
    if(abs(angleVal.x) < 45){      // |x| < 45
        if(abs(angleVal.y) < 45){      // |y| < 45
            if(angleVal.z > 0){          //  z  > 0
                orientation = ICM20948_FLAT;
            }
            else{                        //  z  < 0
                orientation = ICM20948_FLAT_1;
            }
        }
        else{                         // |y| > 45 
            if(angleVal.y > 0){         //  y  > 0
                orientation = ICM20948_XY;
            }
            else{                       //  y  < 0
                orientation = ICM20948_XY_1;   
            }
        }
    }
    else{                           // |x| >= 45
        if(angleVal.x > 0){           //  x  >  0
            orientation = ICM20948_YX;       
        }
        else{                       //  x  <  0
            orientation = ICM20948_YX_1;
        }
    }
    return orientation;
}

String ICM20948_WE::getOrientationAsString(){
    ICM20948_orientation orientation = getOrientation();
    String orientationAsString = "";
    switch(orientation){
        case ICM20948_FLAT:      orientationAsString = "z up";   break;
        case ICM20948_FLAT_1:    orientationAsString = "z down"; break;
        case ICM20948_XY:        orientationAsString = "y up";   break;
        case ICM20948_XY_1:      orientationAsString = "y down"; break;
        case ICM20948_YX:        orientationAsString = "x up";   break;
        case ICM20948_YX_1:      orientationAsString = "x down"; break;
    }
    return orientationAsString;
}
    
float ICM20948_WE::getPitch(){
    xyzFloat angleVal;
    getAngles(&angleVal);
    float pitch = (atan2(-angleVal.x, sqrt(abs((angleVal.y*angleVal.y + angleVal.z*angleVal.z))))*180.0)/M_PI;
    return pitch;
}
    
float ICM20948_WE::getRoll(){
    xyzFloat angleVal;
    getAngles(&angleVal);
    float roll = (atan2(angleVal.y, angleVal.z)*180.0)/M_PI;
    return roll;
}


/************** Interrupts ***************/

void ICM20948_WE::setIntPinPolarity(ICM20948_intPinPol pol){
    regVal = readRegister8(0, ICM20948_INT_PIN_CFG);
    if(pol){
        regVal |= ICM20948_INT1_ACTL;
    }
    else{
        regVal &= ~ICM20948_INT1_ACTL;
    }
    writeRegister8(0, ICM20948_INT_PIN_CFG, regVal);
}

void ICM20948_WE::enableIntLatch(bool latch){
    regVal = readRegister8(0, ICM20948_INT_PIN_CFG);
    if(latch){
        regVal |= ICM20948_INT_1_LATCH_EN;
    }
    else{
        regVal &= ~ICM20948_INT_1_LATCH_EN;
    }
    writeRegister8(0, ICM20948_INT_PIN_CFG, regVal);
}

void ICM20948_WE::enableClearIntByAnyRead(bool clearByAnyRead){
    regVal = readRegister8(0, ICM20948_INT_PIN_CFG);
    if(clearByAnyRead){
        regVal |= ICM20948_INT_ANYRD_2CLEAR;
    }
    else{
        regVal &= ~ICM20948_INT_ANYRD_2CLEAR;
    }
    writeRegister8(0, ICM20948_INT_PIN_CFG, regVal);
}

void ICM20948_WE::setFSyncIntPolarity(ICM20948_intPinPol pol){
    regVal = readRegister8(0, ICM20948_INT_PIN_CFG);
    if(pol){
        regVal |= ICM20948_ACTL_FSYNC;
    }
    else{
        regVal &= ~ICM20948_ACTL_FSYNC;
    }
    writeRegister8(0, ICM20948_INT_PIN_CFG, regVal);
}

void ICM20948_WE::enableInterrupt(ICM20948_intType intType){
    switch(intType){
        case ICM20948_FSYNC_INT:
            regVal = readRegister8(0, ICM20948_INT_PIN_CFG);
            regVal |= ICM20948_FSYNC_INT_MODE_EN;
            writeRegister8(0, ICM20948_INT_PIN_CFG, regVal); // enable FSYNC as interrupt pin
            regVal = readRegister8(0, ICM20948_INT_ENABLE);
            regVal |= 0x80;
            writeRegister8(0, ICM20948_INT_ENABLE, regVal); // enable wake on FSYNC interrupt
            break;
        
        case ICM20948_WOM_INT:
            regVal = readRegister8(0, ICM20948_INT_ENABLE);
            regVal |= 0x08;
            writeRegister8(0, ICM20948_INT_ENABLE, regVal);
            regVal = readRegister8(2, ICM20948_ACCEL_INTEL_CTRL);
            regVal |= 0x02;
            writeRegister8(2, ICM20948_ACCEL_INTEL_CTRL, regVal);
            break;
        
        case ICM20948_DMP_INT:
            regVal = readRegister8(0, ICM20948_INT_ENABLE);
            regVal |= 0x02;
            writeRegister8(0, ICM20948_INT_ENABLE, regVal);
            break;
        
        case ICM20948_DATA_READY_INT:
            writeRegister8(0, ICM20948_INT_ENABLE_1, 0x01);
            break;
        
        case ICM20948_FIFO_OVF_INT:
            writeRegister8(0, ICM20948_INT_ENABLE_2, 0x01);
            break;
        
        case ICM20948_FIFO_WM_INT:
            writeRegister8(0, ICM20948_INT_ENABLE_3, 0x01);
            break;
    }
}

void ICM20948_WE::disableInterrupt(ICM20948_intType intType){
    switch(intType){
        case ICM20948_FSYNC_INT:
            regVal = readRegister8(0, ICM20948_INT_PIN_CFG);
            regVal &= ~ICM20948_FSYNC_INT_MODE_EN; 
            writeRegister8(0, ICM20948_INT_PIN_CFG, regVal);
            regVal = readRegister8(0, ICM20948_INT_ENABLE);
            regVal &= ~(0x80);
            writeRegister8(0, ICM20948_INT_ENABLE, regVal);
            break;
        
        case ICM20948_WOM_INT:
            regVal = readRegister8(0, ICM20948_INT_ENABLE);
            regVal &= ~(0x08);
            writeRegister8(0, ICM20948_INT_ENABLE, regVal);
            regVal = readRegister8(2, ICM20948_ACCEL_INTEL_CTRL);
            regVal &= ~(0x02);
            writeRegister8(2, ICM20948_ACCEL_INTEL_CTRL, regVal);
            break;
        
        case ICM20948_DMP_INT:
            regVal = readRegister8(0, ICM20948_INT_ENABLE);
            regVal &= ~(0x02);
            writeRegister8(0, ICM20948_INT_ENABLE, regVal);
            break;
        
        case ICM20948_DATA_READY_INT:
            writeRegister8(0, ICM20948_INT_ENABLE_1, 0x00);
            break;
        
        case ICM20948_FIFO_OVF_INT:
            writeRegister8(0, ICM20948_INT_ENABLE_2, 0x00);
            break;
        
        case ICM20948_FIFO_WM_INT:
            writeRegister8(0, ICM20948_INT_ENABLE_3, 0x00);
            break;
    }
}

uint8_t ICM20948_WE::readAndClearInterrupts(){
    uint8_t intSource = 0;
    regVal = readRegister8(0, ICM20948_I2C_MST_STATUS);
    if(regVal & 0x80){
        intSource |= 0x01;
    }
    regVal = readRegister8(0, ICM20948_INT_STATUS);
    if(regVal & 0x08){
        intSource |= 0x02;
    } 
    if(regVal & 0x02){
        intSource |= 0x04;
    } 
    regVal = readRegister8(0, ICM20948_INT_STATUS_1);
    if(regVal & 0x01){
        intSource |= 0x08;
    } 
    regVal = readRegister8(0, ICM20948_INT_STATUS_2);
    if(regVal & 0x01){
        intSource |= 0x10;
    }
    regVal = readRegister8(0, ICM20948_INT_STATUS_3);
    if(regVal & 0x01){
        intSource |= 0x20;
    }
    return intSource;
}

bool ICM20948_WE::checkInterrupt(uint8_t source, ICM20948_intType type){
    source &= type;
    return source;
}
void ICM20948_WE::setWakeOnMotionThreshold(uint8_t womThresh, ICM20948_womCompEn womCompEn){
    regVal = readRegister8(2, ICM20948_ACCEL_INTEL_CTRL);
    if(womCompEn){
        regVal |= 0x01;
    }
    else{
        regVal &= ~(0x01);
    }
    writeRegister8(2, ICM20948_ACCEL_INTEL_CTRL, regVal);
    writeRegister8(2, ICM20948_ACCEL_WOM_THR, womThresh);   
}

/***************** FIFO ******************/

void ICM20948_WE::enableFifo(bool fifo){
    regVal = readRegister8(0, ICM20948_USER_CTRL);
    if(fifo){
        regVal |= ICM20948_FIFO_EN;
    }
    else{
        regVal &= ~ICM20948_FIFO_EN;
    }
    writeRegister8(0, ICM20948_USER_CTRL, regVal);
}

void ICM20948_WE::setFifoMode(ICM20948_fifoMode mode){
    if(mode){
        regVal = 0x01;
    }
    else{
        regVal = 0x00;
    }
    writeRegister8(0, ICM20948_FIFO_MODE, regVal);
}

void ICM20948_WE::startFifo(ICM20948_fifoType fifo){
    fifoType = fifo;
    writeRegister8(0, ICM20948_FIFO_EN_2, fifoType);
}

void ICM20948_WE::stopFifo(){
    writeRegister8(0, ICM20948_FIFO_EN_2, 0);
}

void ICM20948_WE::resetFifo(){
    writeRegister8(0, ICM20948_FIFO_RST, 0x01);
    writeRegister8(0, ICM20948_FIFO_RST, 0x00);
}

int16_t ICM20948_WE::getFifoCount(){
    int16_t regVal16 = static_cast<int16_t>(readRegister16(0, ICM20948_FIFO_COUNT));
    return regVal16;
}

int16_t ICM20948_WE::getNumberOfFifoDataSets(){
    int16_t numberOfSets = getFifoCount();
        
    if((fifoType == ICM20948_FIFO_ACC) || (fifoType == ICM20948_FIFO_GYR)){
        numberOfSets /= 6;
    }
    else if(fifoType==ICM20948_FIFO_ACC_GYR){
        numberOfSets /= 12;
    }
    
    return numberOfSets;
}

void ICM20948_WE::findFifoBegin(){
    uint16_t count = getFifoCount();
    int16_t start = 0;
        
    if((fifoType == ICM20948_FIFO_ACC) || (fifoType == ICM20948_FIFO_GYR)){
        start = count%6;
        for(int i=0; i<start; i++){
            readRegister8(0, ICM20948_FIFO_R_W);
        }
    }
    else if(fifoType==ICM20948_FIFO_ACC_GYR){
        start = count%12;
        for(int i=0; i<start; i++){
            readRegister8(0, ICM20948_FIFO_R_W);
        }
    }
}


/************** Magnetometer **************/

bool ICM20948_WE::initMagnetometer(){
    enableI2CMaster();
    resetMag();
    reset_ICM20948();
    sleep(false);
    writeRegister8(2, ICM20948_ODR_ALIGN_EN, 1); // aligns ODR 
    
    bool initSuccess = false;
    uint8_t tries = 0;
    while(!initSuccess && (tries < 10)){ // max. 10 tries to init the magnetometer
        delay(10);
        enableI2CMaster();
        delay(10);
        
        int16_t whoAmI = whoAmIMag();
       if(! ((whoAmI == AK09916_WHO_AM_I_1) || (whoAmI == AK09916_WHO_AM_I_2))){
           initSuccess = false;
            i2cMasterReset();
            tries++;
        }
        else {
            initSuccess = true;
        }
    }
    if(initSuccess){
        setMagOpMode(AK09916_CONT_MODE_100HZ);
    }
    return initSuccess;
}

uint16_t ICM20948_WE::whoAmIMag(){ 
    uint8_t MSByte = readAK09916Register8_SLV4(AK09916_WIA_1);
    uint8_t LSByte = readAK09916Register8_SLV4(AK09916_WIA_2);
    uint16_t magID = (MSByte<<8) | LSByte;
    return magID;
}

void ICM20948_WE::setMagOpMode(AK09916_opMode opMode){
    writeAK09916Register8_SLV4(AK09916_CNTL_2, opMode);
    delay(10);
    if(opMode!=AK09916_PWR_DOWN){
        enableMagDataRead(AK09916_HXL, 0x08);
    }
}

void ICM20948_WE::resetMag(){
    writeAK09916Register8_SLV4(AK09916_CNTL_3, 0x01);
    delay(100);
}

/************************************************ 
     Private Functions
*************************************************/

void ICM20948_WE::setClockToAutoSelect(){
    regVal = readRegister8(0, ICM20948_PWR_MGMT_1);
    regVal |= 0x01;
    writeRegister8(0, ICM20948_PWR_MGMT_1, regVal);
    delay(10);
}

void ICM20948_WE::correctAccRawValues(xyzFloat *corrAccRawVal){
    corrAccRawVal->x = (corrAccRawVal->x -(accOffsetVal.x / accRangeFactor)) / accCorrFactor.x;
    corrAccRawVal->y = (corrAccRawVal->y -(accOffsetVal.y / accRangeFactor)) / accCorrFactor.y;
    corrAccRawVal->z = (corrAccRawVal->z -(accOffsetVal.z / accRangeFactor)) / accCorrFactor.z;
}

void ICM20948_WE::correctGyrRawValues(xyzFloat *corrGyrRawVal){
    corrGyrRawVal->x -= (gyrOffsetVal.x / gyrRangeFactor);
    corrGyrRawVal->y -= (gyrOffsetVal.y / gyrRangeFactor);
    corrGyrRawVal->z -= (gyrOffsetVal.z / gyrRangeFactor);
}

void ICM20948_WE::switchBank(uint8_t newBank){
    if(newBank != currentBank){
        currentBank = newBank;
        if(!useSPI){
            _wire->beginTransmission(i2cAddress);
            _wire->write(ICM20948_REG_BANK_SEL);
            _wire->write(currentBank<<4);
            _wire->endTransmission();
        }
        else{
            _spi->beginTransaction(mySPISettings);
            digitalWrite(csPin, LOW);
            _spi->transfer(ICM20948_REG_BANK_SEL); 
            _spi->transfer(currentBank<<4);
            digitalWrite(csPin, HIGH);
            _spi->endTransaction();
        }
    }
}

void ICM20948_WE::writeRegister8(uint8_t bank, uint8_t reg, uint8_t val){
    switchBank(bank);
        
    if(!useSPI){
        _wire->beginTransmission(i2cAddress);
        _wire->write(reg);
        _wire->write(val);
        _wire->endTransmission();
    }
    else{
        _spi->beginTransaction(mySPISettings);
        digitalWrite(csPin, LOW);
        _spi->transfer(reg); 
        _spi->transfer(val);
        digitalWrite(csPin, HIGH);
        _spi->endTransaction();
    }
}

void ICM20948_WE::writeRegister16(uint8_t bank, uint8_t reg, int16_t val){
    switchBank(bank);
    int8_t MSByte = static_cast<int8_t>((val>>8) & 0xFF);
    uint8_t LSByte = val & 0xFF;
        
    if(!useSPI){
        _wire->beginTransmission(i2cAddress);
        _wire->write(reg);
        _wire->write(MSByte);
        _wire->write(LSByte);
        _wire->endTransmission();
    }
    else{
        _spi->beginTransaction(mySPISettings);
        digitalWrite(csPin, LOW);
        _spi->transfer(reg); 
        _spi->transfer(val);
        digitalWrite(csPin, HIGH);
        _spi->endTransaction();
    }
}

uint8_t ICM20948_WE::readRegister8(uint8_t bank, uint8_t reg){
    switchBank(bank);
    uint8_t regValue = 0;
    
    if(!useSPI){
        _wire->beginTransmission(i2cAddress);
        _wire->write(reg);
        _wire->endTransmission(false);
        _wire->requestFrom(i2cAddress, static_cast<uint8_t>(1));
        if(_wire->available()){
            regValue = _wire->read();
        }
    }
    else{
        reg |= 0x80;
        _spi->beginTransaction(mySPISettings);
        digitalWrite(csPin, LOW);
        _spi->transfer(reg); 
        regValue = _spi->transfer(0x00);
        digitalWrite(csPin, HIGH);
        _spi->endTransaction();
    }
    return regValue;
}

int16_t ICM20948_WE::readRegister16(uint8_t bank, uint8_t reg){
    switchBank(bank);
    uint8_t MSByte = 0, LSByte = 0;
    int16_t reg16Val = 0;
    
    if(!useSPI){
        _wire->beginTransmission(i2cAddress);
        _wire->write(reg);
        _wire->endTransmission(false);
        _wire->requestFrom(i2cAddress, static_cast<uint8_t>(2));
        if(_wire->available()){
            MSByte = _wire->read();
            LSByte = _wire->read();
        }
    }
    else{
        reg = reg | 0x80;
        _spi->beginTransaction(mySPISettings);
        digitalWrite(csPin, LOW);
        _spi->transfer(reg); 
        MSByte = _spi->transfer(0x00);
        LSByte = _spi->transfer(0x00);
        digitalWrite(csPin, HIGH);
        _spi->endTransaction();
    }
    reg16Val = (MSByte<<8) + LSByte;
    return reg16Val;
}

void ICM20948_WE::readAllData(uint8_t* data){    
    switchBank(0);
    
    if(!useSPI){
        _wire->beginTransmission(i2cAddress);
        _wire->write(ICM20948_ACCEL_OUT);
        _wire->endTransmission(false);
        _wire->requestFrom(i2cAddress, static_cast<uint8_t>(20));
        if(_wire->available()){
            for(int i=0; i<20; i++){
                data[i] = _wire->read();
            }
        }
    }
    else{
        uint8_t reg = ICM20948_ACCEL_OUT | 0x80;
        _spi->beginTransaction(mySPISettings);
        digitalWrite(csPin, LOW);
        _spi->transfer(reg);
        for(int i=0; i<20; i++){
                data[i] = _spi->transfer(0x00);
        }
        digitalWrite(csPin, HIGH);
        _spi->endTransaction();
    }
}

void ICM20948_WE::readICM20948xyzValFromFifo(xyzFloat *xyzResult){
    uint8_t fifoTriple[6];
    switchBank(0);
   
    if(!useSPI){
        _wire->beginTransmission(i2cAddress);
        _wire->write(ICM20948_FIFO_R_W);
        _wire->endTransmission(false);
        _wire->requestFrom(i2cAddress, static_cast<uint8_t>(6));
        if(_wire->available()){
            for(int i=0; i<6; i++){
                fifoTriple[i] = _wire->read();
            }
        }
    }
    else{
        uint8_t reg = ICM20948_FIFO_R_W | 0x80;
        _spi->beginTransaction(mySPISettings);
        digitalWrite(csPin, LOW);
        _spi->transfer(reg);
        for(int i=0; i<6; i++){
                fifoTriple[i] = _spi->transfer(0x00);
        }
        digitalWrite(csPin, HIGH);
        _spi->endTransaction();
    }
    
    xyzResult->x = (static_cast<int16_t>((fifoTriple[0]<<8) + fifoTriple[1])) * 1.0;
    xyzResult->y = (static_cast<int16_t>((fifoTriple[2]<<8) + fifoTriple[3])) * 1.0;
    xyzResult->z = (static_cast<int16_t>((fifoTriple[4]<<8) + fifoTriple[5])) * 1.0;
}

void ICM20948_WE::writeAK09916Register8_SLV4(uint8_t reg, uint8_t val){
    writeRegister8(3, ICM20948_I2C_SLV4_ADDR, AK09916_ADDRESS); // write AK09916
    writeRegister8(3, ICM20948_I2C_SLV4_DO, val);
    writeRegister8(3, ICM20948_I2C_SLV4_REG, reg); // define AK09916 register to be written to
    writeRegister8(3, ICM20948_I2C_SLV4_CTRL, ICM20948_I2C_SLVX_EN);
    while(readRegister8(3, ICM20948_I2C_SLV4_CTRL) & ICM20948_I2C_SLVX_EN){;}
}

uint8_t ICM20948_WE::readAK09916Register8_SLV4(uint8_t reg){ // only as slave 4
    writeRegister8(3, ICM20948_I2C_SLV4_ADDR, AK09916_ADDRESS | AK09916_READ); // read AK09916
    writeRegister8(3, ICM20948_I2C_SLV4_REG, reg); // define AK09916 register to be read
    writeRegister8(3, ICM20948_I2C_SLV4_CTRL, ICM20948_I2C_SLVX_EN);
    while(readRegister8(3, ICM20948_I2C_SLV4_CTRL) & ICM20948_I2C_SLVX_EN){;}
    return readRegister8(3, ICM20948_I2C_SLV4_DI);
}

int16_t ICM20948_WE::readAK09916Register16(uint8_t reg){ // only as slave 0
    int16_t regValue = 0;
    enableMagDataRead(reg, 0x02);
    regValue = readRegister16(0, ICM20948_EXT_SLV_SENS_DATA_00);
    enableMagDataRead(AK09916_HXL, 0x08);
    return regValue;
}

void ICM20948_WE::reset_ICM20948(){
    writeRegister8(0, ICM20948_PWR_MGMT_1, ICM20948_RESET);
    delay(10);  // wait for registers to reset
}

void ICM20948_WE::enableI2CMaster(){
    writeRegister8(0, ICM20948_USER_CTRL, ICM20948_I2C_MST_EN); //enable I2C master
    writeRegister8(3, ICM20948_I2C_MST_CTRL, 0x07); // set I2C clock to 345.60 kHz
    delay(10);
}

void ICM20948_WE::i2cMasterReset(){
    uint8_t regVal = readRegister8(0, ICM20948_USER_CTRL);
    regVal |= ICM20948_I2C_MST_RST;    
    writeRegister8(0, ICM20948_USER_CTRL, regVal);
    delay(10);  
}

void ICM20948_WE::enableMagDataRead(uint8_t reg, uint8_t bytes){
    writeRegister8(3, ICM20948_I2C_SLV0_ADDR, AK09916_ADDRESS | AK09916_READ); // read AK09916
    writeRegister8(3, ICM20948_I2C_SLV0_REG, reg); // define AK09916 register to be read
    writeRegister8(3, ICM20948_I2C_SLV0_CTRL, ICM20948_I2C_SLVX_EN | bytes); //enable read | number of byte
    delay(10);
}
