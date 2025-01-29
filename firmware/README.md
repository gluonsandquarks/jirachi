# Jirachi Firmware

This is the source code for the Jirachi device. It is completely written in C under the [ESP-IDF](https://github.com/espressif/esp-idf) framework.

## Quick Start
> [!NOTE]
> Make sure to have ESP-IDF v5.3.1 or higher installed and configured.
```
$ idf.py set-target esp32c3
$ idf.py build
```
You can upload the binary to your ESP32-C3 device with
```
$ idf.py -p <PORT> flash
```

## Overview
The firmware implements the following:

- I2C driver to completely communicate with and manage the onboard Inertial Measurement Unit (IMU - accelerometer and gyroscope).
- Bluetooth Low Energy driver and event handler running on a separate thread for wireless control and tuning of the PID controller.
- Madgwick Filter sensor-fusion algorithm implementation. Takes reading from the IMU and estimate the device's current attitude.
- A PID controller implementation.
- Motor driver with independent PWM channels, all exposed through a simple API.
- RGB LED driver implemented with the RMT peripheral for precise control, all exposed through a simple API. It also implements a manager for color blending and custom light show sequences.
- Runs on Espressif's fork of FreeRTOS.