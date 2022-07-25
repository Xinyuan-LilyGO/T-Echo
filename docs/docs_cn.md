
## **[English](../README.MD) | 中文**

默认使用示例为**Arduino**,同时也支持[**nRF5-SDK**](https://www.nordicsemi.com/Software-and-Tools/Software/nRF5-SDK/Download)

The default example is **Arduino**, which also supports [**nRF5-SDK**](https://www.nordicsemi.com/Software-and-Tools/Software/nRF5-SDK/Download)


<h3 align = "left">产品 📷:</h3>

|    产品    |                               产品链接                               | 原理图 |
| :--------: | :------------------------------------------------------------------: | :----: |
| [T-Echo]() | [Product link](https://pt.aliexpress.com/item/1005002842456390.html) |        |

<h3 align = "left">引脚 :</h3>

- See [utilities.h](examples/Integration/utilities.h)

<h3 align = "left">应用 :</h3>

- [T-Echo SoftRF](https://github.com/lyusupov/SoftRF/wiki/Badge-Edition)
- [T-Echo Meshtastic](https://github.com/meshtastic/Meshtastic-device/tree/v1.2.42.2759c8d)







## 使用**Arduino IDE**
1. 下载[Arduino IDE](https://www.arduino.cc/en/software)
2. 打开Arduino,打开首选项 -> 添加https://www.adafruit.com/package_adafruit_index.json 到 板安装管理器地址列表
3. 打开板子安装管理器中,等待索引更新完成,选择'Adafruit nRF52 by Adafruit'点击安装
4. 安装完成后,在板子列表中选择'Nordic nRF52840(PCA10056)'
5. 将lib目录中的所有文件夹拷贝到`"C:\User\<YourName>\Documents\Arduino\libraries"`中
6. 打开草图 => 工具 => 端口 ,选择已连接板子的端口,然后点击上传

### 使用**PlatformIO**
1. 安装[VSCODE](https://code.visualstudio.com/)和[Python](https://www.python.org/)
2. 在VSCODE扩展中搜索PlatformIO插件并安装。
3. 安装完成，重新加载后，左下角会多一个小房子图标，点击后即可显示Platformio IDE主页
4. 点击文件->打开文件夹->选择LilyGO-T-ECHO文件夹，点击左下角(√)符号进行编译 (→)代表上传.
5.  如果使用USB下载固件（platformio.ini 配置upload_protocol = nrfutil），在下载前需要双击复位按键进入DFU模式

## 注意事项:
1. 需要使用**lib**目录中的文件,它包括:
   - [`arduino-lmic`](https://github.com/mcci-catena/arduino-lmic)
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

2. 默认使用[Adafruit_nRF52_Arduino](https://github.com/adafruit/Adafruit_nRF52_Arduino),所有出厂已经烧录[Adafruit_nRF52_Bootloader](https://github.com/adafruit/Adafruit_nRF52_Bootloader),如果使用**nRF5-SDK**对板子编程 将会丢失原先Bootloader

3. 如果需要使用**nRF5-SDK**进行编程,请点击链接下载[**nRF5-SDK**](https://www.nordicsemi.com/Software-and-Tools/Software/nRF5-SDK/Download)

4. **Adafruit_nRF52_Arduino**中不支持**NFC**功能,请用[**nRF5-SDK**](https://www.nordicsemi.com/Software-and-Tools/Software/nRF5-SDK/Download)进行编程
5. FLASH将根据供货情况选择MX25R1635FZUIL0或者ZD25WQ16B。使用时注意区别。

## 注意事项 :

1. LoRa设置输出功率后需要设置最低电流。设置sx1262如下 :

```
    // digitalWrite(LoRa_Busy, LOW);
    // set output power to 10 dBm (accepted range is -17 - 22 dBm)
    if (radio.setOutputPower(22) == RADIOLIB_ERR_INVALID_OUTPUT_POWER) {
        Serial.println(F("Selected output power is invalid for this module!"));
        while (true);
    }


    // set over current protection limit to 80 mA (accepted range is 45 - 240 mA)
    // NOTE: set value to 0 to disable overcurrent protection
    if (radio.setCurrentLimit(80) == RADIOLIB_ERR_INVALID_CURRENT_LIMIT) {
        Serial.println(F("Selected current limit is invalid for this module!"));
        while (true);
    }
```