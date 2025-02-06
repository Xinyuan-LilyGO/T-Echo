/**
 * @file      ble.cpp
 * @author    Lewis He (lewishe@outlook.com)
 * @license   MIT
 * @copyright Copyright (c) 2024  ShenZhen XinYuan Electronic Technology Co., Ltd
 * @date      2024-05-04
 *
 */
#include <bluefruit.h>
#include "ble_gap.h"
#include "display.h"

// BLE Service
static BLEDfu  bledfu;  // OTA DFU service
static BLEDis  bledis;  // device information
static BLEUart bleuart; // uart over ble
static BLEBas  blebas;  // battery
String BLE_NAME;
extern xQueueHandle     ledHandler;
/**********************************************
  ____  _      ______
 |  _ \| |    |  ____|
 | |_) | |    | |__
 |  _ <| |    |  __|
 | |_) | |____| |____
 |____/|______|______|

************************************************/

// callback invoked when central connects
void connect_callback(uint16_t conn_handle)
{
    // Get the reference to current connection
    BLEConnection *connection = Bluefruit.Connection(conn_handle);
    char central_name[32] = { 0 };
    connection->getPeerName(central_name, sizeof(central_name));
    Serial.print("Connected to ");
    Serial.println(central_name);
    uint8_t val = CONNECT_LED;
    xQueueSend(ledHandler, &val, portMAX_DELAY);
}

/**
 * Callback invoked when a connection is dropped
 * @param conn_handle connection where this event happens
 * @param reason is a BLE_HCI_STATUS_CODE which can be found in ble_hci.h
 */
void disconnect_callback(uint16_t conn_handle, uint8_t reason)
{
    (void) conn_handle;
    (void) reason;
    Serial.println();
    Serial.print("Disconnected, reason = 0x"); Serial.println(reason, HEX);
    uint8_t val = DISCONNECT_LED;
    xQueueSend(ledHandler, &val, portMAX_DELAY);

}

void setupBLE()
{
    Serial.println("Init Bluefruit");

    Serial.flush();

    delay(200);

    Bluefruit.begin();

    Bluefruit.autoConnLed(false);
    // Config the peripheral connection with maximum bandwidth
    // more SRAM required by SoftDevice
    Bluefruit.setTxPower(4);    // Check bluefruit.h for supported values
    uint8_t mac[6];
    Bluefruit.getAddr(mac);
    BLE_NAME = "T-Echo-";
    BLE_NAME.concat(mac[0]);
    BLE_NAME.concat(mac[1]);
    Bluefruit.setName(BLE_NAME.c_str());

    //Bluefruit.setName(getMcuUniqueID()); // useful testing with multiple central connections
    Bluefruit.Periph.setConnectCallback(connect_callback);
    Bluefruit.Periph.setDisconnectCallback(disconnect_callback);

   
    // Start BLE Battery Service
    blebas.begin();
    // blebas.write(100);

    // Advertising packet
    Bluefruit.Advertising.addFlags(BLE_GAP_ADV_FLAGS_LE_ONLY_GENERAL_DISC_MODE);
    Bluefruit.Advertising.addTxPower();

    // Include bleuart 128-bit uuid
    // Bluefruit.Advertising.addService(bleuart);
    Bluefruit.Advertising.addService(blebas);

    // Secondary Scan Response packet (optional)
    // Since there is no room for 'Name' in Advertising packet
    Bluefruit.ScanResponse.addName();

    /* Start Advertising
     * - Enable auto advertising if disconnected
     * - Interval:  fast mode = 20 ms, slow mode = 152.5 ms
     * - Timeout for fast mode is 30 seconds
     * - Start(timeout) with timeout = 0 will advertise forever (until connected)
     *
     * For recommended advertising interval
     * https://developer.apple.com/library/content/qa/qa1931/_index.html
     */
    Bluefruit.Advertising.restartOnDisconnect(true);
    Bluefruit.Advertising.setInterval(32, 244);    // in unit of 0.625 ms
    Bluefruit.Advertising.setFastTimeout(30);      // number of seconds in fast mode
    Bluefruit.Advertising.start(0);                // 0 = Don't stop advertising after n seconds

}



void loopBle()
{
    // if (Bluefruit.connected()) {
    // }
}
