<div align="center" markdown="1">
  <img src="../.github/LilyGo_logo.png" alt="LilyGo logo" width="100"/>
</div>

<h1 align = "center">🌟 T-Echo Firmware 🌟</h1>


## 📖 Table of Contents

- [📖 Table of Contents](#-table-of-contents)
- [📋 Firmware Overview](#-firmware-overview)
- [📁 Firmware Directory Structure](#-firmware-directory-structure)
- [🔄 How to Update Firmware](#-how-to-update-firmware)

---

## 📋 Firmware Overview

| Firmware File      | Description                                                        |
| :----------------- | :----------------------------------------------------------------- |
| `firmware-xxx.uf2` | Includes LoRa transceiver, BLE connection, and all hardware test items. For BLE testing, download [nRF Connect](https://www.nordicsemi.com/Products/Development-tools/nrf-connect-for-mobile) from Google Play. |
| `gps_factory.uf2`  | Used to diagnose hardware issues and test GPS functionality.     |

---

## 📁 Firmware Directory Structure

```
firmware/
├── factory
│   ├── SoftRF-firmware-v1.3-nRF52.uf2      # SoftRF factory pre-flashed version
│   └── firmware-t-echo-inkhud-2.7.13.uf2   # Meshtastic factory pre-flashed version
├── firmware-433M.uf2   # Factory test firmware 433MHz
├── firmware-868M.uf2   # Factory test firmware 868MHz
├── firmware-915M.uf2   # Factory test firmware 915MHz
├── firmware-920M.uf2   # Factory test firmware 920MHz
├── firmware-923M.uf2   # Factory test firmware 923MHz
├── gps_factory.uf2     # GPS NMEA output firmware
```

---

## 🔄 How to Update Firmware

1. Connect the device to your computer using a USB-C to USB-A cable (do not use USB-C to USB-C).
2. Double-click the top-left button on the device. If it doesn't work, try again.
3. Once the device disk appears on your computer, drag the `*.uf2` firmware file onto the disk and wait for the file to copy.
4. The device will boot normally.
