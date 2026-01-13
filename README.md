# ESP32 Dual Flow Monitoring System with IoT & TFT Display

## Overview

This project implements a real-time **dual-channel liquid flow monitoring system** using an ESP32 microcontroller. It measures inlet and outlet flow rates using hall-effect flow sensors, displays data on a TFT screen, logs historical data to an SD card, and uploads total volume readings to the ThingSpeak cloud platform.

The system supports:
- Real-time flow rate measurement (L/s, L/min, L/hour)
- Cumulative volume tracking
- On-device visualization using ILI9341 TFT
- Data logging to SD card
- Cloud synchronization using WiFi (ThingSpeak)
- Push-button controlled unit switching
- Power-loss recovery via SD-stored last values

---

## Hardware Used

- ESP32 Dev Board  
- YF-S201 (or similar) Flow Sensors ×2 (Inlet & Outlet)  
- ILI9341 2.8" TFT Display  
- Micro SD Card Module  
- Push Button  
- External Power Supply  
- Pipes & Pump (for real testing)

Board wiring diagram is provided in the repository.

---

## Features

- Dual flow measurement with pulse-interrupt accuracy
- Unit switching: L/sec, L/min, L/hour
- Non-volatile data logging to SD card
- Auto WiFi reconnection
- ThingSpeak cloud integration
- Pump ON/OFF detection
- Industrial-style HMI interface

---

## Cloud Integration

Data is uploaded to ThingSpeak every 5 minutes:

- Field 1 → Inlet total volume (L)
- Field 2 → Outlet total volume (L)

---

## Setup Instructions

1. Install Arduino IDE
2. Install ESP32 board support
3. Install required libraries (see requirements below)
4. Edit WiFi and ThingSpeak credentials in:

```cpp
const char* ssid = "YOUR_WIFI_NAME";
const char* password = "YOUR_WIFI_PASSWORD";
const char* thingSpeakWriteAPIKey = "YOUR_THINGSPEAK_WRITE_API_KEY";
