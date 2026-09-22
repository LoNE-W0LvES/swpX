# swpX - Smart Water Pump System (ESP32-S3)

Next-generation, production-grade embedded firmware for the **ESP32-S3** microcontroller powering an automated, intelligent water pump and tank management system with OLED UI, multi-protocol cloud synchronization, and machine learning-assisted inflow prediction.

---

## 🌟 Key Highlights

- **ESP32-S3 Hardware Acceleration:** Built for the ESP32-S3 DevKitC-1 with dual-core processing and low-latency peripherals.
- **OLED Graphical Interface:** Real-time water level, pump state, inflow rate, and network indicators displayed on an SSD1306 0.96" I2C OLED screen with anti-flicker double buffering.
- **Predictive Edge Intelligence:** Integrated `MLPredictor` engine for estimating water consumption patterns and inflow rates.
- **Triple-Protocol Sync:** Switchable networking layers supporting **MQTT** (`PubSubClient`), **WebSockets**, and **REST API**.
- **Comprehensive Motor & Safety Protection:** Dry-run detection, emergency overflow shutdown (98%), and rapid cycle prevention.
- **Simulation Ready:** Includes Wokwi simulation configuration (`diagram.json`, `wokwi.toml`) for complete browser-based emulation.

---

## 🎛️ System Architecture & Hardware Pinout

```
+-------------------------------------------------------------+
|                       ESP32-S3 DevKit                       |
|                                                             |
|  [GPIO 21/22] <---> I2C OLED Display (SSD1306 128x64)      |
|  [GPIO 5/18]  <---> Ultrasonic Sensor (HC-SR04 / JSN-SR04T) |
|  [GPIO 4]     <---> Pump Control Relay                      |
|  [GPIO 12-14] <---> Tactile Buttons (Menu, Select, Override)|
|  [WiFi Radio] <---> MQTT / WebSocket / REST / Local AP      |
+-------------------------------------------------------------+
```

---

## ⚙️ Core Modules

| Module | Files | Responsibility |
| :--- | :--- | :--- |
| **Sensor & Measurement** | `sensor.cpp/.h`, `tank_calculator.cpp/.h` | Ultrasonic distance sensing, dead-zone filtering (25cm offset), cylindrical/rectangular volume math, inflow/outflow (L/min) calculation |
| **Pump Controller** | `pump_controller.cpp/.h` | Threshold automation (start at ≤ 20%, stop at ≥ 90%), dry-run cutoff (5 min timeout), overflow emergency cutoff (98%), minimum run (60s) & rest (120s) |
| **Display Manager** | `display_manager.cpp/.h` | SSD1306 OLED rendering, status screens, setup wizard, signal strength and error indicators |
| **ML Predictor** | `ml_predictor.cpp/.h` | Edge ML inference for predicting future tank levels and water inflow dynamics |
| **IoT Client & Sync** | `iot_client.cpp/.h`, `iot_mqtt.cpp`, `iot_websocket.cpp`, `iot_restapi.cpp` | Multi-protocol communication engine supporting telemetry publishing and remote control |
| **Local Web Server** | `webserver_local.cpp/.h` | Built-in asynchronous HTTP server (`ESPAsyncWebServer`) with captive portal and local control dashboard |
| **WiFi & OTA** | `wifi_manager.cpp/.h`, `ota_updater.cpp/.h` | Robust reconnection logic, AP fallback, and over-the-air firmware updates |
| **Storage Manager** | `storage_manager.cpp/.h` | Non-volatile NVS flash storage for tank calibration, thresholds, and WiFi credentials |

---

## 🛠️ Building and Flashing

### Prerequisites
- [PlatformIO Core](https://platformio.org/) or the PlatformIO IDE extension in VS Code.

### Commands
```bash
# Clone the repository
git clone https://github.com/LoNE-W0LvES/swpX.git
cd swpX

# Compile the firmware
pio run

# Flash to connected ESP32-S3 board
pio run -t upload

# Open serial monitor
pio device monitor -b 115200
```

### Running in Wokwi Simulator
This repository includes `diagram.json` and `wokwi.toml`. You can test the firmware directly in the [Wokwi](https://wokwi.com/) simulator without physical hardware.