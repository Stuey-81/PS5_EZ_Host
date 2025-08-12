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

## Build Instructions
1. Install [Arduino IDE](https://www.arduino.cc/en/software) and ESP32 board support.
2. Open `WIP.ino` in Arduino IDE.
3. Select your ESP32-S3 board and matching partition scheme.
4. Upload the sketch to the board.
5. Flash the SPIFFS image containing the `data/` folder.

## Downloads
Compiled binaries (`firmware.bin` and `spiffs.bin`) are available in the **[Releases](../../releases)** section.  
Source code is provided so users can review and compile themselves.

## License
This project is licensed under the [MIT License](LICENSE).
