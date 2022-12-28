

#define BUTTON_YES    UserButton_Pin
#define BUTTON_NO     Touch_Pin

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


void setupDisplay();
void enableBacklight(bool en);
void configVDD(void);
void boardInit();
void LilyGo_logo(void);

SPIClass        *dispPort  = nullptr;
SPIClass        *rfPort    = nullptr;
GxIO_Class      *io        = nullptr;
GxEPD_Class     *display   = nullptr;

uint32_t        blinkMillis = 0;
uint8_t rgb = 0;
uint8_t offsetX = 0;
uint8_t offsetY = 20;
uint8_t GapX = 13;
uint8_t GapY = 21;
uint8_t updateWindow_num = 0;

bool EP_Bl_state = true;
/*
 * This sketch demonstrate the central API(). An additional bluefruit
 * that has blehid as peripheral is required for the demo.
 */
#include <bluefruit.h>

// Polling or callback implementation
#define POLLING       0

BLEClientHidAdafruit hid;

// Last checked report, to detect if there is changes between reports
hid_keyboard_report_t last_kbd_report = { 0 };
hid_mouse_report_t last_mse_report = { 0 };

bool Clear_screen_flag = true;

void scan_callback(ble_gap_evt_adv_report_t *report);
void connect_callback(uint16_t conn_handle);
void connection_secured_callback(uint16_t conn_handle);
void disconnect_callback(uint16_t conn_handle, uint8_t reason);
void processKeyboardReport(hid_keyboard_report_t *report);
void keyboard_report_callback(hid_keyboard_report_t *report);
void pairing_complete_callback(uint16_t conn_handle, uint8_t auth_status);
bool pairing_passkey_callback(uint16_t conn_handle, uint8_t const passkey[6], bool match_request);
void setup()
{
    Serial.begin(115200);
    delay(200);
    // while ( !Serial ) delay(10);   // for nrf52840 with native usb
    boardInit();

    Serial.println("Bluefruit52 Central HID (Keyboard + Mouse) Example");
    Serial.println("--------------------------------------------------\n");

    // Initialize Bluefruit with maximum connections as Peripheral = 0, Central = 1
    // SRAM usage required by SoftDevice will increase dramatically with number of connections
    Bluefruit.begin(0, 1);

    Bluefruit.setName("Bluefruit52 Central");


// To use dynamic PassKey for pairing, we need to have
    // - IO capacities at least DISPPLAY
    // - Register callback to display/print dynamic passkey for central
    // For complete mapping of the IO Capabilities to Key Generation Method, check out this article
    // https://www.bluetooth.com/blog/bluetooth-pairing-part-2-key-generation-methods/
    Bluefruit.Security.setIOCaps(true, false, false); // display = true, yes/no = true, keyboard = false
    Bluefruit.Security.setPairPasskeyCallback(pairing_passkey_callback);

    // Set complete callback to print the pairing result
    Bluefruit.Security.setPairCompleteCallback(pairing_complete_callback);

    // Set connection secured callback, invoked when connection is encrypted
    Bluefruit.Security.setSecuredCallback(connection_secured_callback);


    // Init BLE Central Hid Serivce
    hid.begin();

#if POLLING == 0
    hid.setKeyboardReportCallback(keyboard_report_callback);
#endif

    // Increase Blink rate to different from PrPh advertising mode
    Bluefruit.setConnLedInterval(250);

    // Callbacks for Central
    Bluefruit.Central.setConnectCallback(connect_callback);
    Bluefruit.Central.setDisconnectCallback(disconnect_callback);

    // Set connection secured callback, invoked when connection is encrypted
    // Bluefruit.Security.setSecuredCallback(connection_secured_callback);

    /* Start Central Scanning
     * - Enable auto scan if disconnected
     * - Interval = 100 ms, window = 80 ms
     * - Don't use active scan
     * - Filter only accept HID service in advertising
     * - Start(timeout) with timeout = 0 will scan forever (until connected)
     */
    Bluefruit.Scanner.setRxCallback(scan_callback);
    Bluefruit.Scanner.restartOnDisconnect(true);
    Bluefruit.Scanner.setInterval(160, 80); // in unit of 0.625 ms
    Bluefruit.Scanner.filterService(hid);   // only report HID service
    Bluefruit.Scanner.useActiveScan(true);
    Bluefruit.Scanner.start(0);             // 0 = Don't stop scanning after n seconds


    delay(200);
    // LilyGo_logo();
    // delay(2000);
    display->setRotation(3);
    display->fillScreen(GxEPD_WHITE);
    display->setTextColor(GxEPD_BLACK);
    display->setFont(&FreeMonoBold12pt7b);

    display->fillScreen(GxEPD_WHITE);
    display->setCursor(0, 20);
    display->print("Scan ble keyboard"); //crashes here
    display->update();
    // display->update();
    // display->setCursor(13 * 14, 20 * 10);



}

void loop()
{

    if (millis() - blinkMillis > 1000) {
        // Serial.print("loop ");
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

#if POLLING == 1
    // nothing to do if hid not discovered
    if ( !hid.discovered() ) {
        Serial.print("discovered ");
        return;
    }

    /*------------- Polling Keyboard  -------------*/
    hid_keyboard_report_t kbd_report;

    // Get latest report
    hid.getKeyboardReport(&kbd_report);

    processKeyboardReport(&kbd_report);


    // polling interval is 5 ms
    delay(5);
#endif

}


/**
 * Callback invoked when scanner pick up an advertising data
 * @param report Structural advertising data
 */
void scan_callback(ble_gap_evt_adv_report_t *report)
{
    // Since we configure the scanner with filterUuid()
    // Scan callback only invoked for device with hid service advertised
    // Connect to the device with hid service in advertising packet
    Bluefruit.Central.connect(report);
}

/**
 * Callback invoked when an connection is established
 * @param conn_handle
 */
void connect_callback(uint16_t conn_handle)
{
    BLEConnection *conn = Bluefruit.Connection(conn_handle);

    Serial.println("Connected");

    Serial.print("Discovering HID  Service ... ");

    if ( hid.discover(conn_handle) ) {
        Serial.println("Found it");

        // HID device mostly require pairing/bonding
        conn->requestPairing();
    }

    else {
        Serial.println("Found NONE");

        // disconnect since we couldn't find blehid service
        conn->disconnect();
    }
}

void connection_secured_callback(uint16_t conn_handle)
{
    BLEConnection *conn = Bluefruit.Connection(conn_handle);

    if ( !conn->secured() ) {
        // It is possible that connection is still not secured by this time.
        // This happens (central only) when we try to encrypt connection using stored bond keys
        // but peer reject it (probably it remove its stored key).
        // Therefore we will request an pairing again --> callback again when encrypted
        conn->requestPairing();
    } else {
        Serial.println("Secured");

        // https://www.bluetooth.com/specifications/gatt/viewer?attributeXmlFile=org.bluetooth.characteristic.hid_information.xml
        uint8_t hidInfo[4];
        hid.getHidInfo(hidInfo);

        Serial.printf("HID version: %d.%d\n", hidInfo[0], hidInfo[1]);
        Serial.print("Country code: "); Serial.println(hidInfo[2]);
        Serial.printf("HID Flags  : 0x%02X\n", hidInfo[3]);

        // BLEClientHidAdafruit currently only supports Boot Protocol Mode
        // for Keyboard and Mouse. Let's set the protocol mode on prph to Boot Mode
        hid.setBootMode(false);

        // Enable Keyboard report notification if present on prph
        if ( hid.keyboardPresent() ) hid.enableKeyboard();

        // Enable Mouse report notification if present on prph
        if ( hid.mousePresent() ) hid.enableMouse();

        Serial.println("Ready to receive from peripheral");
        display->fillScreen(GxEPD_WHITE);
        display->setCursor(0, 20);
        display->print("Connect to the BLE keyboard");
        display->update();

        offsetX = 0;
        offsetY = 20;
        Clear_screen_flag = true;

    }
}

/**
 * Callback invoked when a connection is dropped
 * @param conn_handle
 * @param reason is a BLE_HCI_STATUS_CODE which can be found in ble_hci.h
 */
void disconnect_callback(uint16_t conn_handle, uint8_t reason)
{
    (void) conn_handle;
    (void) reason;

    Serial.print("Disconnected, reason = 0x"); Serial.println(reason, HEX);
}

void processKeyboardReport(hid_keyboard_report_t *report)
{

    // Check with last report to see if there is any changes
    if ( memcmp(&last_kbd_report, report, sizeof(hid_keyboard_report_t)) ) {
        bool shifted = false;
        bool alt = false;

        if ( report->modifier  ) {
            if ( report->modifier & (KEYBOARD_MODIFIER_LEFTCTRL | KEYBOARD_MODIFIER_RIGHTCTRL) ) {
                Serial.print("Ctrl ");
            }

            if ( report->modifier & (KEYBOARD_MODIFIER_LEFTSHIFT | KEYBOARD_MODIFIER_RIGHTSHIFT) ) {
                Serial.print("Shift ");

                shifted = true;
            }


            if ( report->modifier & (KEYBOARD_MODIFIER_LEFTALT | KEYBOARD_MODIFIER_RIGHTALT) ) {
                Serial.print("Alt ");
            }


        }

        for (uint8_t i = 0; i < 6; i++) {
            uint8_t kc = report->keycode[i];
            char ch = 0;

            if ( kc < 128 ) {
                ch = shifted ? hid_keycode_to_ascii[kc][1] : hid_keycode_to_ascii[kc][0];
            } else {
                // non-US keyboard !!??
            }

            // Printable
            if (ch) {
                if ( Clear_screen_flag ) {
                    display->fillScreen(GxEPD_WHITE);
                    display->update();
                    Clear_screen_flag = false;

                }
                if ( updateWindow_num == 5 ) {
                    // display->update();
                    updateWindow_num = 0;
                }
                display->setCursor(offsetX, offsetY);
                display->print(ch);
                if (ch == 0x08) {//keyboard Backspace
                    display->fillRect(offsetX - GapX + 1, offsetY - GapY + 6, GapX, GapY, GxEPD_WHITE);
                    // display->update();
                    if (offsetX > GapX) {
                        offsetX -= GapX;
                    } else {
                        if (offsetY > GapY) {
                            /
                            offsetY -= GapY;
                            offsetX = GapX * 15;
                        } else {
                            offsetY = GapY;
                            offsetX = 0;
                        }
                    }
                } else if (ch == 0x0d) { //keyboard ENTER
                    if (offsetY > GapY * 8) {
                        offsetY = GapY;
                        offsetX = 0;
                        display->fillScreen(GxEPD_WHITE);
                    } else {//A newline  换行
                        offsetY += GapY;
                        offsetX = 0;
                    }

                } else {
                    offsetX += GapX;
                    if (offsetX > (GapX * 14)) {//That's the end of the line
                        if (offsetY > GapY * 8) { //It's the last line
                            offsetY = 20;
                            offsetX = 0;
                            Clear_screen_flag = true;
                            // display->fillScreen(GxEPD_WHITE);
                        } else { //A newline
                            offsetX = 0;
                            offsetY +=  GapY;
                        }
                    }
                }
                // display->updateWindow(offsetX - GapX + 1, offsetY - GapY + 6, GapX, GapY, GxEPD_WHITE);
                updateWindow_num++;
                display->update();
                Serial.print(ch);
            }
        }
    }

    // update last report
    memcpy(&last_kbd_report, report, sizeof(hid_keyboard_report_t));
}

void keyboard_report_callback(hid_keyboard_report_t *report)
{
    processKeyboardReport(report);
}

void pairing_complete_callback(uint16_t conn_handle, uint8_t auth_status)
{
    BLEConnection *conn = Bluefruit.Connection(conn_handle);

    if (auth_status == BLE_GAP_SEC_STATUS_SUCCESS) {
        Serial.println("Succeeded");
    } else {
        Serial.println("Failed");

        // disconnect
        conn->disconnect();
    }


}

bool pairing_passkey_callback(uint16_t conn_handle, uint8_t const passkey[6], bool match_request)
{
    Serial.println("Pairing Passkey");
    Serial.printf("    %.3s %.3s\n\n", passkey, passkey + 3);

    // match_request means peer wait for our approval (return true)
    if (match_request) {
        Serial.println("Do you want to pair");
        Serial.println("Press Button Left to decline, Button Right to Accept");

        // timeout for pressing button
        uint32_t start_time = millis();

        // wait until either button is pressed (30 seconds timeout)
        while ( digitalRead(BUTTON_YES) && digitalRead(BUTTON_NO) ) {
            if ( millis() > start_time + 30000 ) break;
        }

        if ( 0 == digitalRead(BUTTON_YES) ) return true;

        if ( 0 == digitalRead(BUTTON_NO) ) return false;


        return false;
    }

    return true;
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
