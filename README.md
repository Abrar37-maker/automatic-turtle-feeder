# Automatic Turtle Feeder

An intelligent, IoT-enabled automated feeding and monitoring system designed for pet turtles. Powered by the **ESP32**, this system combines dual-motor actuation, ultrasonic feed-level telemetry, infrared bowl-safety interlocks, and an embedded local web dashboard.

---

## 📌 Project Overview
The **Automatic Turtle Feeder** solves the challenge of regular, accurate, and hygienic feeding for pet turtles. Overfeeding or missed feedings severely compromise water quality and turtle health. This device automates dispensing schedules, monitors hopper storage, verifies bowl alignment before opening food gates, and provides real-time telemetry over local Wi-Fi.

### Key Features
- **Dual-Actuation Dispensing**:
  - **SG90 Micro Servo**: Operates a metered dispensing gate (0° sealed shut, 35° open).
  - **DC Agitation Motor (L298N H-Bridge)**: High-frequency bi-directional "jiggle" agitation (120ms forward, 60ms pause, 120ms backward) that prevents dry pellet bridging and chute clumping.
- **Smart Safety & Telemetry**:
  - **Ultrasonic Level Telemetry (HC-SR04)**: Dynamic 5-sample moving-average distance filter converts hopper depth into an accurate remaining food percentage.
  - **Infrared Bowl Safety Interlock**: Detects food dish placement; disables the dispensing servo and triggers an audio-visual alarm if the bowl is missing.
  - **DS1302 Real-Time Clock**: Hardware RTC keeps accurate time across power cycles for automated feeding intervals.
- **Dual User Interface**:
  - **Local Hardware UI**: 16x2 I2C LCD display, RGB status LEDs, piezo buzzer, and manual control/calibration buttons.
  - **Embedded IoT Web Dashboard**: Asynchronous HTTP web server serving a responsive dark-mode web app with live JSON telemetry and non-volatile flash logs (`Preferences.h`).

---

## 👥 Team Information
- **Project Type**: Individual Project
- **Developer / Student**: **Abrar Zaheen Suham**
- **Email**: `abrarzaheen21@gmail.com`
- **Platform**: ESP32 Dev Module (Dev1)

---

## 🛠️ Individual Contributions
As an individual project, all facets of engineering, implementation, and documentation were performed solely by **Abrar Zaheen Suham**:
- **Firmware Architecture**: Designed the non-blocking state machine in C++ / Arduino framework managing actuation delays and timers.
- **Actuation & Motor Control**: Programmed PWM-driven H-Bridge directional control for hopper jittering and calibrated the micro servo gate sweep angle.
- **Sensor Calibration & Digital Filtering**: Implemented a moving-average filter and outlier rejection algorithm on ultrasonic sonar echo timings to stabilize food level readings.
- **IoT & Web Engineering**: Built the onboard ESP32 web server, custom dark-theme HTML/CSS/JavaScript dashboard, REST API endpoints (`/api/status`, `/api/logs`), and persistent NVS flash storage for historical feeding logs.
- **Circuit Design & Hardware Integration**: Designed the unified power and logic pinout interconnecting the ESP32, L298N motor driver, DS1302 RTC, I2C LCD, IR sensor, HC-SR04 sonar, buzzer, and indicator LEDs.

---

## 🎥 Project Demonstration
- **Video Link**: [![Watch Demo](https://img.shields.io/badge/YouTube-Watch%20Demo-red?style=for-the-badge&logo=youtube)](https://youtu.be/kWpr0NYigxg)
  > *Direct link: [https://youtu.be/kWpr0NYigxg](https://youtu.be/kWpr0NYigxg)*

---

## 🔬 Motor Selection Justification

The project integrates two distinct motors—an **SG90 Micro Servo** and a **DC Motor driven by an L298N Dual H-Bridge**—each deliberately selected to fulfill distinct physical and mechanical requirements:

### 1. SG90 Micro Servo (Dispenser Gate Control)
- **Precise Closed-Loop Angular Control**: The dispensing gate requires reliable, repeatable angular positioning between `0°` (fully sealed) and `35°` (calibrated opening). Because the SG90 features an internal potentiometer and feedback control circuit, the ESP32 can command exact positions via standard 50 Hz PWM without external encoders or homing limit switches.
- **Holding Torque vs. Compact Form Factor**: With ~1.8 kg·cm of stall torque at 5V and weighing only 9 grams, the SG90 provides sufficient force to open and shut against loose food pellets while maintaining a lightweight footprint that fits cleanly above the feeding chute.
- **Rapid Transit & Clean Shutoff**: Operating at ~0.10 sec/60°, the servo opens quickly for a precise 3-second feeding window and snaps shut, avoiding food dribble and overfeeding.
- **Safety Detach Feature**: When the IR sensor detects the feeding bowl is missing, the firmware calls `myServo.detach()`, removing active power from the motor windings to conserve power and prevent unintended dispensing.

### 2. DC Agitation Motor with L298N Dual H-Bridge (Hopper Anti-Jamming & Agitation)
- **Overcoming Pellet Bridging & Friction**: Dry turtle pellets are irregular, light, and prone to "arching" or "bridging" over narrow dispenser funnels under gravity. Continuous rotation or static vibration often packs the pellets tighter.
- **Rapid Bi-Directional Jitter Cycle**: By pairing a DC motor with an **L298N H-Bridge**, the firmware executes high-frequency directional reversal (`jiggleForward()` for 120ms &rarr; pause for 60ms &rarr; `jiggleBackward()` for 120ms) under PWM speed control (`MOTOR_PWM_VALUE = 130`). This back-and-forth agitation breaks pellet clumps dynamically, ensuring continuous gravity feed through the servo gate.
- **Inductive Surge & Current Handling**: DC motors produce heavy inrush currents and inductive back-EMF spikes during rapid reversals. The L298N driver isolates high-current motor power from the ESP32 logic rail, handling up to 2A per channel with built-in protection.
- **Cost-Efficiency & Reliability**: A DC motor + L298N solution provides far higher instantaneous torque and durability for rapid shaking than miniature stepper motors at a fraction of the cost.

---

## ⚙️ Setup & Reproduction Instructions

### 1. Hardware Bill of Materials (BOM)
- **Microcontroller**: ESP32 Dev Module (Dev1, 30-pin or 38-pin)
- **Gate Actuator**: TowerPro SG90 9g Micro Servo
- **Agitator Motor**: 5V–9V DC Toy/Gear Motor
- **Motor Driver**: L298N Dual H-Bridge Driver Module
- **Distance Sensor**: HC-SR04 Ultrasonic Sonar Module
- **Safety Sensor**: TCRT5000 / Standard Active-LOW Infrared Proximity Module
- **Real-Time Clock**: DS1302 3-Wire RTC Module with CR2032 coin cell
- **Display**: 16x2 Character LCD with PCF8574 I2C Backpack
- **User Interface**: 3x Momentary Tactile Push Buttons, 1x Active/Passive Buzzer, 3x 5mm LEDs (Red, Green, Blue) with 220Ω resistors
- **Power Supply**: 5V 2A DC Regulated Supply (or dual rail 5V logic / 9V motor supply)

### 2. Pinout & Wiring Connections
See detailed schematic tables in [`hardware/wiring_and_pinout.md`](./hardware/wiring_and_pinout.md).

| Subsystem | Component | Component Pin | ESP32 GPIO |
| :--- | :--- | :--- | :--- |
| **Servo Gate** | SG90 Servo | Signal (PWM) | **GPIO 13** |
| **Agitator Driver** | L298N Motor Driver | ENA (PWM) / IN1 / IN2 | **GPIO 4 / GPIO 18 / GPIO 19** |
| **Food Telemetry** | Ultrasonic (HC-SR04) | TRIG / ECHO | **GPIO 25 / GPIO 26** |
| **Bowl Interlock** | IR Proximity Sensor | Digital OUT | **GPIO 32** |
| **Real-Time Clock** | DS1302 RTC | DAT / CLK / RST | **GPIO 2 / GPIO 5 / GPIO 15** |
| **Display** | 16x2 LCD (I2C) | SDA / SCL | **GPIO 21 / GPIO 22** |
| **Controls** | Push Buttons | Manual / Reset / Auto | **GPIO 14 / GPIO 27 / GPIO 34** |
| **Status LEDs** | LEDs (220Ω) | Red / Green / Blue | **GPIO 16 / GPIO 17 / GPIO 23** |
| **Alarm** | Buzzer | Positive (+) | **GPIO 33** |

### 3. Software Installation & Dependencies
1. Install [Arduino IDE](https://www.arduino.cc/en/software) (or VS Code with PlatformIO).
2. In Arduino IDE Settings, add the ESP32 board manager URL:
   ```text
   https://raw.githubusercontent.com/espressif/arduino-esp32/gh-pages/package_esp32_index.json
   ```
3. Install the required libraries via the Arduino Library Manager:
   - `ESP32Servo` by Kevin Harrington
   - `LiquidCrystal_I2C` by Frank de Brabander
   - `Rtc by Makuna` (for DS1302)
   - Built-in ESP32 libraries: `WiFi`, `WebServer`, `Preferences`, `Wire`

### 4. Flashing & Running
1. Open [`src/automatic-turtle-feeder.ino`](./src/automatic-turtle-feeder.ino) in Arduino IDE.
2. Update the Wi-Fi credentials under Section 1 if using a different local network:
   ```cpp
   const char* IOT_WIFI_SSID = "Your_WiFi_SSID";
   const char* IOT_WIFI_PASSWORD = "Your_WiFi_Password";
   ```
3. Select board **"DOIT ESP32 DEVKIT V1"** (or **"ESP32 Dev Module"**).
4. Connect the ESP32 via USB and select the corresponding COM port.
5. Click **Upload**.
6. Open the Serial Monitor at **115200 baud** to view boot logs and the assigned local IP address for the IoT dashboard (e.g. `http://192.168.x.x`).

---

## 📁 Repository Structure
```
automatic-turtle-feeder/
├── cad/                          # 3D printable models (STL, STEP, CAD files)
│   └── README.md                 # Specifications for printed mechanical components
├── hardware/                     # Electrical schematics & wiring specifications
│   └── wiring_and_pinout.md      # Pin mapping, power rails, and electrical specifications
├── src/                          # Firmware source code
│   └── automatic-turtle-feeder.ino # Complete ESP32 firmware with IoT Web Dashboard
├── docs/                         # Additional documentation and assets
├── .gitignore                    # Build and system artifact ignore rules
└── README.md                     # Comprehensive project documentation & submission
```
