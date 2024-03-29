# VerbalEyes Scroll Controller

## Introduction
The VerbalEyes Scroll Controller is a small WiFi connected controller for controlling teleprompter scrolling on a VerbalEyes servers.
When connected to a VerbalEyes project, the scrolling speed can be controlled using a dial and the scroll position can be reset back to the top of the document with a button press.
It is built on the ESP8266 platform but the core API can be used separately to implement it on other platforms as well.



## LED indicator
The built-in status LED on the ESP8266 vaguely indicates what the speed controller is doing.
To get more detailed information about what is happening, connect the device to a computer to read its logs.
The easiest way to read the logs is to use either the [bash script](./tools/configure_bash) or the [web interface](./tools/configure_web), but it is also possible to read logs manually from the serial port (instructions for manually reading and writing over serial [here](./tools/configure_bash/README.md#manual-configuration)).

| Blinks | Description |
| --- | -
| Slow | If the built-in status LED blinks once every 8 seconds, that means the scroll controller is connected to a server and ready to transmit data.
| Fast | When the built-in status LED blinks continuously, the scroll controller is either not connected to a server and/or is in the middle of processing configuration data.

## Configuration
The scroll controller is configured from a computer by connecting to it over USB and either using the [bash script](./tools/configure_bash), the [web interface](./tools/configure_web) or manually sending data to it (instructions for manually reading and writing over serial [here](./tools/configure_bash/README.md#manual-configuration)).

## Build
This project requires wiring up the hardware and flashing the firmware.

### Components
* 1x Wemos D1 Mini
* 1x Potentiometer (resistance doesn't really matter)
* 1x Button (optional)
* Some wires (red, black and other)

### Wiring
TODO: Add details on how the board should be wired (potentiometer: terminal vcc, terminal gnd, wiper)

### Flashing Firmware
Flashing the firmware is done using the Arduino IDE.
1. Download the Arduino IDE if it is not installed already.
2. Add support for the ESP8266 by following [these instructions](https://arduino-esp8266.readthedocs.io/en/latest/installing.html#using-git-version)
3. Open the file `teleprompter-arduino.ino` in the Arduino IDE.
4. Compile and upload the project to the device.
This includes plugging it in over USB, selecting the correct serial port and maybe more.



## Core Library
The core library can be used to implement the scroll controller on other platforms.
It is written in C and works in C99 or newer as well as C++.
More details on how the core API works can be found [here](./src/README.md).
