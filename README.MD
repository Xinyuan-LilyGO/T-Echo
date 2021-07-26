<h1 align = "center">🌟LilyGo T-ECHO🌟</h1>

## **English | [中文](docs/docs_cn.md)**

## Using Arduino IDE
1. Download and install the CH340 driver 
   - [CH340 Drivers](http://www.wch-ic.com/search?q=ch340&t=downloads)
2. Open Arduino, open preferences-> add https://www.adafruit.com/package_adafruit_index.json to the board installation manager address list
3. Open the board installation manager, wait for the index update to complete, select 'Adafruit nRF52 by Adafruit' and click install
4. After the installation is complete, select 'Nordic nRF52840(PCA10056)' in the board list
5. Copy all the folders in the lib directory to `"C:\User\<YourName>\Documents\Arduino\libraries"`
6. Open the sketch => tool => port, select the port of the connected board, and click upload





<h3 align = "left">Product 📷:</h3>

|  Product   |                                                                                                Product  Link                                                                                                 |
| :--------: | :----------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------: |
| [T-Echo]() | [AliExpress](https://pt.aliexpress.com/item/1005002842456390.html)  [Amazon](https://www.amazon.com/dp/B097T4QGCT?ref=myi_title_dp) [Tindie](https://www.tindie.com/products/lilygo/lilygo-t-echo-nrf52840/) |

<h3 align = "left">PinOut :</h3>

![](image/T-ECHO.jpg)


<h3 align = "left">Application :</h3>

- [T-Echo SoftRF](https://github.com/lyusupov/SoftRF/wiki/Badge-Edition)
- [T-Echo Meshtastic](https://github.com/meshtastic/Meshtastic-device/tree/v1.2.42.2759c8d)





## Precautions:
1. Need to use the files in the **lib** directory, it includes:
   - `AceButton` 
   - `Adafruit_BME280_Library`   
   - `Adafruit_BusIO`        
   - `Adafruit_EPD`          
   - `AceButton` 
   - `Adafruit-GFX-Library`   
   - `Button2`        
   - `GxEPD`            
   - `PCF8563_Library `               
   - `RadioLib`     
   - `SerialFlash_ID539 `               
   - `SoftSPI`   
   - `TinyGPSPlus`

2. Pins on boards are not compatible with the official SDK.If you want to use the SDK, pay special attention to the pin definition.

3. Use [Adafruit_nRF52_Arduino](https://github.com/adafruit/Adafruit_nRF52_Arduino) by default, so [Adafruit_nRF52_Bootloader](https://github.com/adafruit/Adafruit_nRF52_Bootloader) has been burned in the factory, if you use **nRF5-SDK** Programming the board will lose the original bootloader

4. If you need to use **nRF5-SDK** for programming, please click the link to download [**nRF5-SDK**](https://www.nordicsemi.com/Software-and-Tools/Software/nRF5-SDK/Download)

5. **NFC Function** is not supported in **Adafruit_nRF52_Arduino**, please use [**nRF5-SDK**](https://www.nordicsemi.com/Software-and-Tools/Software/nRF5-SDK/Download) for programming
6. Flash will choose MX25R1635FZUIL0 or ZD25WQ16B according to the availability.Pay attention to the difference when using.
7. If you need to burn boatload. You can refer to [Burning boatload](bootloader/README.MD)

