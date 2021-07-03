
## **[English](../README.MD) | 中文**

默认使用示例为**Arduino**,同时也支持[**nRF5-SDK**](https://www.nordicsemi.com/Software-and-Tools/Software/nRF5-SDK/Download)

## 如何使用
### 使用**Arduino IDE**
1. 下载并且安装CH340驱动程序
   - [CH340 Drivers](http://www.wch-ic.com/search?q=ch340&t=downloads)
2. 打开Arduino,打开首选项 -> 添加https://www.adafruit.com/package_adafruit_index.json 到 板安装管理器地址列表
3. 打开板子安装管理器中,等待索引更新完成,选择'Adafruit nRF52 by Adafruit'点击安装
4. 安装完成后,在板子列表中选择'Nordic nRF52840(PCA10056)'
5. 将lib目录中的所有文件夹拷贝到`"C:\User\<YourName>\Documents\Arduino\libraries"`中
6. 打开草图 => 工具 => 端口 ,选择已连接板子的端口,然后点击上传

### 使用**PlatformIO**，直接打开即可,在初次使用会自动下载**Adafruit_nRF52_Arduino**

## 注意事项:
1. 需要使用**lib**目录中的文件,它包括:
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