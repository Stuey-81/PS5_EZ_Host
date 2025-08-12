# PS5_EZ_Host by _-stuey-_
Modern UMTX2 ESP32-S3 Web Utility Host (v1.0.3)

<p align="center">
  <img src="screenshot_main.webp" alt="PS5 EZ Host UI Screenshot" width="600">
</p>

---

## What Is This?

PS5_EZ_Host is a self-contained, no-fuss platform for PlayStation 5 firmware 1.xx–5.xx, built on the UMTX2 project and optimized for ESP32-S3 boards (16MB flash). This host offers:

- Clean two-stage interface (launch → payloads) based on idlesauce UMTX2  
- Built-in file manager (`/admin.html`) for hot-swapping `.bin` / `.elf` payloads  
- SPIFFS-based hosting — no SD card required  
- Gzip-capable payload support  
- Auto-flashing via `auto_flash.exe` (no Arduino or Python required) — just plug in, it detects the board/COM port and flashes  
- Compatible with 16MB ESP32-S3 boards using CP210x USB

---

## Flashing Instructions

1. Plug your ESP32-S3 board into a USB port  
   *(CP210x USB-UART recommended — install the VCP driver if Windows doesn’t detect it.)*

2. Download the latest **Release** from this repo (contains `auto_flash.exe` and the images).

3. Double-click `auto_flash.exe`.

4. Let it flash all four images:
   - Bootloader
   - Partition Table
   - Firmware
   - SPIFFS

---

# PS5_EZ_Host

ESP32-S3 based self-hosted web utility platform for PlayStation 5 browser tools.

![Main Screen](screenshot_main.webp)  
![Alternate View](index2.webp)

## Overview
PS5_EZ_Host runs entirely from an ESP32-S3 board, serving a lightweight HTML/JS interface to the PS5 web browser.  
It provides a central place to host and launch browser-based tools, with an integrated admin page for managing files stored on the device.

## Features
- Clean, responsive browser UI  
- Integrated file manager for adding/removing web files  
- Works completely offline once set up  
- Supports `.bin.gz` and `.elf` payload formats

## Downloads
Compiled binaries are available in the **[Releases](../../releases)** section.  
Source code is provided so users can review and compile themselves if they wish.

## License
This project is licensed under the [MIT License](LICENSE).
