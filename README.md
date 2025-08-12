# PS5_EZ_Host

ESP32-S3 based self-hosted web utility platform for PlayStation 5 browser tools.

![Main Screen](screenshot_main.webp)
![Alternate View](index2.webp)

---

## Flashing Instructions (One-Click Method – Recommended)

1. Plug your ESP32-S3 board into a USB port.  
   *(CP210x USB-UART is common; install the VCP driver if Windows doesn’t detect it.)*

2. Download the latest **Release** from this repo — it includes:
   - `auto_flash.exe`
   - `bootloader.bin`
   - `partition-table.bin`
   - `firmware.bin`
   - `spiffs.bin`

3. Double-click `auto_flash.exe`.

4. The flasher will automatically write all four images to your board:
   - **Bootloader**
   - **Partition Table**
   - **Firmware**
   - **SPIFFS**

> No Arduino IDE or Python tools required — everything needed is in the Release package.

---

## Manual Flashing (Advanced / Developers)

If you want to flash without `auto_flash.exe`, you can use [esptool.py](https://github.com/espressif/esptool) or similar.

Example command:
```bash
esptool.py --chip esp32s3 --port COMx --baud 921600 write_flash ^
  0x0000    bootloader.bin ^
  0x8000    partition-table.bin ^
  0x10000   firmware.bin ^
  0x310000  spiffs.bin
