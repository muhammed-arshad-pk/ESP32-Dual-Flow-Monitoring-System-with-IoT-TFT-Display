# ESP32 Dual Flow Monitoring System with IoT & TFT Display

## Overview

This project implements a real-time **dual-channel liquid flow monitoring system** using an ESP32 microcontroller for field-scale IoT deployment. It measures inlet and outlet flow rates using calibrated **YF-S201 hall-effect sensors**, displays live data on a **2.8" ILI9341 TFT**, logs historical measurements to an SD card, and synchronizes cumulative volume to the **ThingSpeak cloud**.

The system supports:

- Real-time flow measurement (L/s, L/min, L/hour)  
- Cumulative volume tracking with power-loss recovery  
- On-device HMI visualization (TFT)  
- SD-card based non-volatile data logging  
- Cloud synchronization via WiFi (ThingSpeak)  
- Field deployment with continuous operation  

---

## Hardware Used

- ESP32 Dev Board  
- YF-S201 Flow Sensors ×2 (Inlet & Outlet)  
- ILI9341 2.8" TFT Display  
- Micro SD Card Module  
- Push Button (unit switching)  
- External Power Supply  
- Pipes & Pump (real hydraulic testing)

---

## Calibration & Accuracy

- Calibration method: **1-liter volumetric reference test**
- Pulse-to-volume constant tuned experimentally  
- Achieved accuracy: **±3–5% over 1–30 L/min range**  
- Sampling rate: **1 Hz real-time flow acquisition**  
- Continuous operation validated under **24×7 field testing**

---

## Features

- Dual-channel flow sensing using interrupt-based pulse counting  
- Unit switching: L/sec, L/min, L/hour  
- SD-based persistent storage with power-loss recovery  
- TFT-based real-time visualization (industrial HMI style)  
- WiFi auto-reconnection and cloud sync  
- Pump ON/OFF detection and anomaly awareness  

---

## Cloud Integration

Data uploaded to **ThingSpeak** every 5 minutes:

- Field 1 → Inlet cumulative volume (Liters)  
- Field 2 → Outlet cumulative volume (Liters)  

End-to-end latency (sensor → cloud): **< 300 ms (WiFi local network)**  
Daily data volume: **~17,000+ samples/day per channel**

---

## System Reliability

- Continuous runtime: **24×7 multi-day field operation**
- Power stability via regulated supply and brown-out protection  
- Data integrity ensured through SD-based checkpoint recovery  
- Verified under varying pump speeds and pressure conditions  

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
happy
