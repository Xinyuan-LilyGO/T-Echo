#include "Wire.h"
#include "Arduino.h"
#include "MPU9250.h"
MPU9250 mpu;

#include "utilities.h"
#include <SPI.h>
#include <Wire.h>
#include <GxEPD.h>
//#include <GxGDEP015OC1/GxGDEP015OC1.h>    // 1.54" b/w
//#include <GxGDEH0154D67/GxGDEH0154D67.h>  // 1.54" b/w
#include <GxDEPG0150BN/GxDEPG0150BN.h>  // 1.54" b/w 

#include GxEPD_BitmapExamples
#include <Fonts/FreeMonoBold12pt7b.h>
#include <GxIO/GxIO_SPI/GxIO_SPI.h>
#include <GxIO/GxIO.h>
#include <Fonts/FreeMonoBold9pt7b.h>
void setupDisplay();
void enableBacklight();
void configVDD(void);
void boardInit();
SPIClass        *dispPort  = nullptr;
SPIClass        *rfPort    = nullptr;
GxIO_Class      *io        = nullptr;
GxEPD_Class     *display   = nullptr;
uint32_t        blinkMillis = 0;
uint8_t rgb = 0;


int mpu9250_last = 0;
int mpu9250_state = 1;
void mpu9250_test();

void setup()
{

    Serial.begin(115200);
    delay(200);
    boardInit();
    delay(200);
    display->setRotation(3);
    Serial.println("setup");

    Wire.setPins(SDA_Pin, SCL_Pin);
    Wire.begin();
    delay(500);
    if (!mpu.setup(0x68)) {  // change to your own address

        mpu9250_state = 0;
    }
    mpu.verbose(false);



    display->setCursor(15, 25);
    display->setFont(&FreeMonoBold9pt7b);
    display->fillScreen(GxEPD_WHITE);
    display->println("ePaper SeftTest");
    display->drawFastHLine(0, display->getCursorY() - 5, display->width(), GxEPD_BLACK);
    display->println();
    display->setFont(&FreeMonoBold12pt7b);
    display->print("[MPU9250]    ");
    display->println(mpu9250_state  ? "+" : "-");
    display->update();
    delay(1000);
    display->fillScreen(GxEPD_WHITE);
    display->update();
    display->setFont(&FreeMonoBold12pt7b);
}


void loop()
{
    mpu9250_test();

}



void mpu9250_test()
{

    if (mpu.update()) {
        if (millis() - mpu9250_last > 500) {
            display->fillScreen(GxEPD_WHITE);
            display->fillRect(0, 0, GxEPD_WIDTH, GxEPD_HEIGHT, GxEPD_WHITE);
            display->setCursor(0, 25);
            display->println("mpu9250_test: ");
            if (mpu.update()) {

                //  display->setCursor(0, 50);
                display->print("Yaw: ");
                display->println(mpu.getYaw());
                //  display->setCursor(0, 75);
                display->print("Pitch: ");
                display->println(mpu.getPitch());
                // display->setCursor(0, 90);
                display->print("Roll: ");
                display->println(mpu.getRoll());
            }
            display->updateWindow(0, 0, GxEPD_WIDTH, GxEPD_HEIGHT, false);
            mpu9250_last = millis();

        }
    }
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
    display->setRotation(3);
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

}
