
# Week 1 - Raspberry Pi GPIO LED Control

## Description
This project controls an external LED using Raspberry Pi GPIO and the libgpiod library.

## Hardware
- Raspberry Pi 4 Model B
- Red LED
- 220/330 ohm resistor
- Breadboard
- Jumper wires

## GPIO Connections
- GPIO4 (Physical Pin 7): LED output
- GND (Physical Pin 9): Ground

## Requirements
- Raspberry Pi OS
- GCC compiler
- libgpiod development library

## Compile
gcc led_control.c -o led_control -lgpiod

## Run
sudo ./led_control

## Stop
Press Ctrl+C.
