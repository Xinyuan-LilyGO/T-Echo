/*
 * @Description: Sleep_Display
 * @Author: LILYGO_L
 * @Date: 2024-08-07 17:27:50
 * @LastEditTime: 2024-09-09 16:17:24
 * @License: GPL 3.0
 */
#include <Wire.h>
#include "Adafruit_EPD.h"
#include "pin_config.h"
#include "RadioLib.h"
#include "wiring.h"
#include <bluefruit.h>
#include "Adafruit_SPIFlash.h"
#include "Display_Fonts.h"

SPIFlash_Device_t GD25LQ128DSIGR =
    {
        total_size : (1UL << 21), /* 2 MiB */
        start_up_time_us : 12000,
        manufacturer_id : 0xBA,
        memory_type : 0x60,
        capacity : 0x15,
        max_clock_speed_mhz : 32,
        quad_enable_bit_mask : 0x02,
        has_sector_protection : false,
        supports_fast_read : true,
        supports_qspi : true,
        supports_qspi_writes : true,
        write_status_register_split : false,
        single_status_byte : false,
        is_fram : false,
    };

SPIClass Custom_SPI_0(NRF_SPIM0, SCREEN_MISO, SCREEN_SCLK, SCREEN_MOSI);
Adafruit_SSD1681 display(SCREEN_WIDTH, SCREEN_HEIGHT, SCREEN_DC, SCREEN_RST,
                         SCREEN_CS, SCREEN_SRAM_CS, SCREEN_BUSY, &Custom_SPI_0);

SPISettings spiSettings;
SPIClass Custom_SPI_3(NRF_SPIM3, SX1262_MISO, SX1262_SCLK, SX1262_MOSI);
SX1262 radio = new Module(SX1262_CS, SX1262_DIO1, SX1262_RST, SX1262_BUSY, Custom_SPI_3, spiSettings);

// QSPI
Adafruit_FlashTransport_QSPI flashTransport(GD25LQ128DSIGR_SCLK, GD25LQ128DSIGR_CS,
                                            GD25LQ128DSIGR_IO0, GD25LQ128DSIGR_IO1,
                                            GD25LQ128DSIGR_IO2, GD25LQ128DSIGR_IO3);
Adafruit_SPIFlash flash(&flashTransport);

void Display_End(void)
{
    pinMode(SCREEN_DC, INPUT);
    pinMode(SCREEN_RST, INPUT_PULLUP);
    pinMode(SCREEN_CS, INPUT_PULLUP);
    pinMode(SCREEN_BUSY, INPUT);
    pinMode(SCREEN_SCLK, INPUT);
    pinMode(SCREEN_MOSI, INPUT);

    Custom_SPI_0.end();
}

void setup()
{
    Serial.begin(115200);
    // while (!Serial)
    // {
    //     delay(100);
    // }
    Serial.println("Ciallo");

    pinMode(nRF52840_BOOT, INPUT_PULLUP);

    pinMode(VDD3_3_EN, OUTPUT);
    digitalWrite(VDD3_3_EN, HIGH);

    pinMode(LED_1, OUTPUT);
    pinMode(LED_2, OUTPUT);
    pinMode(LED_3, OUTPUT);
    digitalWrite(LED_1, LOW);
    digitalWrite(LED_2, LOW);
    digitalWrite(LED_3, LOW);

    pinMode(SCREEN_BL, OUTPUT);
    digitalWrite(SCREEN_BL, HIGH);

    pinMode(GPS_WAKEUP, OUTPUT);
    pinMode(GPS_RST, OUTPUT);
    pinMode(GPS_1PPS, INPUT_PULLUP);
    digitalWrite(GPS_WAKEUP, HIGH);
    digitalWrite(GPS_RST, HIGH);

    display.begin();
    display.setFont(&FreeSans9pt7b);

    // initialize SX1262 with default settings
    Serial.println("[SX1262] Initializing ... ");
    Custom_SPI_3.begin();
    Custom_SPI_3.setClockDivider(SPI_CLOCK_DIV2);
    if (radio.begin() != ERR_NONE)
    {
        Serial.println("SX1262 initialization failed");
    }
    else
    {
        Serial.println("SX1262 initialization successful");
    }

    if (flash.begin(&GD25LQ128DSIGR) == false)
    {
        Serial.println("Flash initialization failed");
    }
    else
    {
        Serial.println("Flash initialization successful");
    }

    flashTransport.setClockSpeed(32000000UL, 0);

    // Enable the protocol stack
    //  if (Bluefruit.begin() == false)
    //  {
    //      Serial.println("BLE initialization failed");
    //  }
    //  Serial.println("BLE initialization successful");

    display.setFont(&FreeSans9pt7b);
    display.fillScreen(EPD_WHITE);
    display.setCursor(10, 60);
    display.setTextColor(EPD_BLACK);
    display.setTextSize(1);
    display.print("All functions are open normally");
    display.display(true);
    delay(5000);

    digitalWrite(SCREEN_BL, LOW);
    pinMode(SCREEN_BL, INPUT_PULLDOWN);

    display.fillScreen(EPD_WHITE);
    display.setCursor(10, 60);
    display.setTextColor(EPD_BLACK);
    display.setTextSize(1);
    display.print("1.Turn off screen backlight");
    display.display(true);
    delay(5000);

    Serial.end();

    display.fillScreen(EPD_WHITE);
    display.setCursor(10, 60);
    display.setTextColor(EPD_BLACK);
    display.setTextSize(1);
    display.print("2.Close USBCDC serial port");
    display.display(true);
    delay(5000);

    radio.sleep();
    Custom_SPI_3.end();
    pinMode(SX1262_MISO, INPUT);
    pinMode(SX1262_MOSI, INPUT);
    pinMode(SX1262_SCLK, INPUT);
    pinMode(SX1262_CS, INPUT_PULLUP);
    pinMode(SX1262_DIO1, INPUT);
    pinMode(SX1262_RST, INPUT_PULLUP);
    pinMode(SX1262_BUSY, INPUT);

    display.fillScreen(EPD_WHITE);
    display.setCursor(10, 60);
    display.setTextColor(EPD_BLACK);
    display.setTextSize(1);
    display.print("3.Lora enters sleep mode");
    display.display(true);
    delay(5000);

    flashTransport.runCommand(0xB9); // Flash Deep Sleep
    flash.end();
    pinMode(GD25LQ128DSIGR_SCLK, INPUT);
    pinMode(GD25LQ128DSIGR_CS, INPUT_PULLUP);
    pinMode(GD25LQ128DSIGR_IO0, INPUT);
    pinMode(GD25LQ128DSIGR_IO1, INPUT);
    pinMode(GD25LQ128DSIGR_IO2, INPUT);
    pinMode(GD25LQ128DSIGR_IO3, INPUT);

    display.fillScreen(EPD_WHITE);
    display.setCursor(10, 60);
    display.setTextColor(EPD_BLACK);
    display.setTextSize(1);
    display.print("4.Flash enters deep sleep mode");
    display.display(true);
    delay(5000);

    digitalWrite(GPS_WAKEUP, LOW);
    pinMode(GPS_WAKEUP, INPUT_PULLDOWN);
    digitalWrite(GPS_RST, LOW);
    pinMode(GPS_RST, INPUT_PULLDOWN);
    pinMode(GPS_1PPS, INPUT_PULLUP);
    pinMode(GPS_UART_RX, INPUT_PULLUP);
    pinMode(GPS_UART_TX, INPUT_PULLUP);

    display.fillScreen(EPD_WHITE);
    display.setCursor(10, 60);
    display.setTextColor(EPD_BLACK);
    display.setTextSize(1);
    display.print("5.GPS enters sleep mode");
    display.display(true);
    delay(5000);

    digitalWrite(IIC_SDA, HIGH);
    digitalWrite(IIC_SCL, HIGH);
    pinMode(IIC_SDA, INPUT_PULLUP);
    pinMode(IIC_SCL, INPUT_PULLUP);

    display.fillScreen(EPD_WHITE);
    display.setCursor(10, 60);
    display.setTextColor(EPD_BLACK);
    display.setTextSize(1);
    display.print("6.Close IIC");
    display.display(true);
    delay(5000);

    digitalWrite(LED_1, HIGH);
    digitalWrite(LED_2, HIGH);
    digitalWrite(LED_3, HIGH);
    pinMode(LED_1, INPUT_PULLUP);
    pinMode(LED_2, INPUT_PULLUP);
    pinMode(LED_3, INPUT_PULLUP);

    display.fillScreen(EPD_WHITE);
    display.setCursor(10, 60);
    display.setTextColor(EPD_BLACK);
    display.setTextSize(1);
    display.print("7.Close LED");
    display.display(true);
    delay(5000);

    display.fillScreen(EPD_WHITE);
    display.setCursor(10, 60);
    display.setTextColor(EPD_BLACK);
    display.setTextSize(1);
    display.print("8.NRF52840 enters 20 second light sleep");
    display.display(true);

    Display_End();
    digitalWrite(VDD3_3_EN, LOW);
    pinMode(VDD3_3_EN, INPUT_PULLDOWN);

    for (int i = 0; i < 30; i++)
    {
        waitForEvent();
        // The delay is necessary, and it will not enter the sleep state without it.
        delay(1000);
    }

    pinMode(VDD3_3_EN, OUTPUT);
    digitalWrite(VDD3_3_EN, HIGH);
    display.begin();

    display.fillScreen(EPD_WHITE);
    display.setCursor(10, 60);
    display.setTextColor(EPD_BLACK);
    display.setTextSize(1);
    display.print("9.NRF52840 enters deep sleep");
    display.display(true);

    Display_End();
    digitalWrite(VDD3_3_EN, LOW);
    pinMode(VDD3_3_EN, INPUT_PULLDOWN);

    systemOff(nRF52840_BOOT, LOW);
}

void loop()
{
}
