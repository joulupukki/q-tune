# Q-Tune Chromatic Tuner
This is an in-progress project with the goal of creating a fully open-source chromatic tuner in a standard pedal enclosure for use on pedalboards and based on the ESP32 MCU.

The project uses the Q DSP Library for frequency detection and LVGL 9 for the graphical user interface.

The project now uses a [Waveshare ESP32-S3 Touch 2.8" 240x320](https://www.waveshare.com/product/mcu-tools/development-boards/esp32/esp32-s3-touch-lcd-2.8.htm?sku=27690) screen because of its clean cover glass that's already attached. It helps make a very professional look in a guitar pedal. Currently the capacitive touch isn't being used.

The project first used a Heltec ESP32-S3 with a smaller display, graduated to the Cheap Yellow Display (ESP32-2432S028R/ESP32-WROOM-32), and is now using the Waveshare.

## Setup

1. Install Visual Studio Code
2. Install the Visual Studio Code ESP-IDF extension
3. Configure the ESP-IDF extension by using the express or advanced mode
    - Be sure to choose **v5.3** as the version of ESP-IDF to use
    - You may need to use advanced mode to be able to select the correct location for python3
    - If pip error, navigate to: C:\Espressif\tools\idf-python\3.11.2> and run: python -m ensurepip --upgrade
4. Make sure you have `git` installed
5. Clone this github project
6. From a terminal go into the project and do this:
    ```
    git submodule update --init --recursive
    ```
7. Open the `q-tune` folder in VS Code
8. Set your ESP32 target (esp32s3)
    - Open the Command Palette (Command+Shift+P on a Mac) and select `ESP-IDF: Set Espressif Device Target`
    - Select `esp32s3`
    - Select the `ESP32-S3 chip (via ESP-PROG)` option
9. Select the port to use
    - Plug in your ESP32 dev board and wait for a few seconds
    - Open the Command Palette (Command+Shift+P on a Mac) and select `ESP-IDF: Select Port to Use (COM, tty, usbserial)`
    - Select your port from the drop-down list that appears
    - Windows Only: If you receive an error while trying to locate the serial/COM port that your ESP32 is connected to (or it doesn't show up), you may need to install the driver for the CH340C UART chip that the ESP32-CYD uses for USB communication. The driver is available from the manufacturer here: https://www.wch-ic.com/downloads/CH341SER_ZIP.html
        - After installing the driver and connecting the CYD, re-run the "Select Port" command
10. Attempt to build the software
    - Use the Command Palette and select `ESP-IDF: Build your Project`
11. Assuming step #9 worked, Build and install the software
    - Use the Command Palette and select `ESP-IDF: Build, Flash, and Start a Monitor on your Device`
    - To stop monitoring the output, press Control+T, then X

## Demo

Better smoothing and pre-amp circuit 19 Mar 2025:
[![Demo Video](https://img.youtube.com/vi/D6ZLS6r_rAo/0.jpg)](https://www.youtube.com/watch?v=D6ZLS6r_rAo)

We're getting closer to releasing the first version. Here's a demo of how it's looking on 13 Mar 2025:
[![Demo Video](https://img.youtube.com/vi/pk4hbDYdYKE/0.jpg)](https://youtube.com/live/pk4hbDYdYKE)

Here's a simple demo of how the project is coming along as of 10 Dec 2024:
[![Demo Video](https://img.youtube.com/vi/im-Qe8d9LSk/0.jpg)](https://youtu.be/im-Qe8d9LSk)

Here's an older demo, more of a proof-of-concept on a smaller screen using a Heltec ESP32-S3 (20 Aug 2024):

[![Demo Video](https://img.youtube.com/vi/XWTicIlTI_k/0.jpg)](https://youtu.be/XWTicIlTI_k)

## License

This software is licensed under the GNU General Public License (GPL) for open-source use. A commercial license is available for those who wish to use this software in proprietary or closed-source projects. For more details, please contact btimothy [at] gmail [dot] com.