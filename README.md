# Automatic Turtle Feeder

An automated feeding system engineered to dispense food portions accurately for turtles at scheduled intervals.

---

## 📌 Project Overview
The **Automatic Turtle Feeder** is an embedded mechatronics project designed to automate the feeding process for pet turtles and aquatic reptiles. The system ensures regular feeding, prevents overfeeding or water fouling, and allows turtle owners to maintain feeding routines effortlessly.

### Key Features
- **Precise Portion Control**: Metered food dispensing mechanism to prevent overfeeding and tank contamination.
- **Automated Scheduling**: Timed intervals for regular feeding routines.
- **Moisture-Resistant Design**: Enclosed food hopper to keep pellets dry in humid aquarium environments.
- **Reliable Actuation**: Custom motor drive designed for torque, food pellet agitation, and jam prevention.

---

## 👥 Team Information
| Name | Student / Member ID | Role / Specialization | Contact / Email |
| :--- | :--- | :--- | :--- |
| **Abrar Zaheen Suham** | *[Insert ID]* | Team Lead / *[Insert Role]* | `abrarzaheen21@gmail.com` |
| *[Member 2 Name]* | *[Insert ID]* | *[Insert Role]* | *[Insert Email]* |
| *[Member 3 Name]* | *[Insert ID]* | *[Insert Role]* | *[Insert Email]* |

---

## 🛠️ Individual Contributions
- **Abrar Zaheen Suham**:
  - *[e.g., System architecture, firmware development, circuit integration, testing]*
- **[Member 2 Name]**:
  - *[e.g., 3D CAD modeling, mechanical design of the hopper and dispenser, 3D printing]*
- **[Member 3 Name]**:
  - *[e.g., Power management, testing & calibration, documentation, video editing]*

---

## 🎥 Project Demonstration
- **Video Link**: [![Watch Demo](https://img.shields.io/badge/YouTube-Watch%20Demo-red?style=for-the-badge&logo=youtube)](https://www.youtube.com/)
  > *Direct link: [Insert YouTube Link Here]*

---

## ⚙️ Setup & Reproduction Instructions

### 1. Hardware Requirements
- **Microcontroller**: *[e.g., Arduino Uno / ESP32 / Raspberry Pi Pico / STM32]*
- **Actuators/Motors**: *[e.g., SG90 Servo / 28BYJ-48 Stepper Motor + ULN2003 / DC Gear Motor]*
- **Sensors/RTC**: *[e.g., DS3231 RTC Module / Infrared sensor]*
- **Power Supply**: *[e.g., 5V 2A DC Adapter or Li-ion Battery]*
- **Chassis/Dispenser**: 3D-printed parts located in [`cad/`](./cad/)

### 2. Software & Dependencies
- **IDE**: Arduino IDE / PlatformIO / VS Code
- **Libraries**:
  - *[List required libraries, e.g. `<Servo.h>`, `<Wire.h>`, `<RTClib.h>`]*

### 3. Wiring & Assembly
1. Mount the 3D-printed hopper and dispenser body using M3 screws.
2. Connect the motor and sensors according to the schematics in [`hardware/`](./hardware/).
3. Connect the microcontroller to your computer via USB.

### 4. Running the Code
1. Open the source code located in [`src/`](./src/).
2. Select your board and COM port in the IDE.
3. Compile and upload the sketch to the microcontroller.
4. Power the system and verify the dispensing cycle.

---

## 🔬 Motor Selection Justification
> *[Detailed engineering justification for chosen motors]*

### Chosen Motor: *[e.g., 28BYJ-48 Stepper Motor with ULN2003 Driver / SG90 Micro Servo / NEMA 17]*
- **Torque Requirements**: *[Explanation of why the motor has sufficient torque to rotate the auger/drum without stalling under the weight of pellets]*
- **Positioning & Control Accuracy**: *[Why this motor allows exact dispensing portions (e.g., step angle / angular position control)]*
- **Power Consumption & Form Factor**: *[Compact size to fit the enclosure, low current draw compatible with small power supplies]*
- **Cost & Availability**: *[Budget-friendly, durable, and widely accessible for prototyping]*
- **Jam Prevention**: *[Ability to reverse or sweep back and forth to clear pellet blockages]*

---

## 📁 Repository Structure
```
automatic-turtle-feeder/
├── cad/                  # 3D models (STL, STEP, CAD design files)
├── hardware/             # Circuit schematics, wiring diagrams, pinouts, BOM
├── src/                  # Firmware and microcontroller source code
├── docs/                 # Documentation, photos, diagrams
├── .gitignore            # Git ignore file for temporary & build artifacts
└── README.md             # Project documentation & submission details
```
