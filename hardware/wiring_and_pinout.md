# Hardware Wiring and Pinout Specification

## 1. System Architecture
The Automatic Turtle Feeder is driven by an **ESP32 Dev Module (ESP32-WROOM-32 / Dev1)** running embedded firmware with FreeRTOS background tasks for local control and asynchronous IoT monitoring.

---

## 2. Pinout Connection Table

| Subsystem | Component | Pin on Component | ESP32 GPIO Pin | Electrical Specs / Notes |
| :--- | :--- | :--- | :--- | :--- |
| **Actuation** | SG90 Micro Servo | PWM (Orange/Yellow) | **GPIO 13** | 5V Power; 50Hz PWM signal |
| **Actuation** | L298N Motor Driver | ENA (PWM speed) | **GPIO 4** | 8-bit PWM (default speed: 130/255) |
| **Actuation** | L298N Motor Driver | IN1 (Direction) | **GPIO 18** | Logic HIGH/LOW for H-bridge |
| **Actuation** | L298N Motor Driver | IN2 (Direction) | **GPIO 19** | Logic HIGH/LOW for H-bridge |
| **Display** | 16x2 LCD (I2C Adapter) | SDA | **GPIO 21** | 3.3V / 5V I2C bus (Address `0x27`) |
| **Display** | 16x2 LCD (I2C Adapter) | SCL | **GPIO 22** | I2C Clock |
| **Timekeeping** | DS1302 RTC | DAT / IO | **GPIO 2** | 3-Wire serial interface |
| **Timekeeping** | DS1302 RTC | CLK / SCLK | **GPIO 5** | Serial Clock |
| **Timekeeping** | DS1302 RTC | RST / CE | **GPIO 15** | Chip Enable / Reset |
| **Sensing** | Ultrasonic (HC-SR04) | TRIG | **GPIO 25** | 10µs trigger pulse |
| **Sensing** | Ultrasonic (HC-SR04) | ECHO | **GPIO 26** | Echo pulse width timing |
| **Sensing** | IR Obstacle/Proximity | OUT | **GPIO 32** | Digital LOW when bowl is aligned |
| **UI / Input** | Push Button 1 (Manual Feed) | Signal | **GPIO 14** | `INPUT_PULLUP` to GND |
| **UI / Input** | Push Button 2 (Sonar Reset) | Signal | **GPIO 27** | `INPUT_PULLUP` to GND |
| **UI / Input** | Push Button 3 (Auto Mode) | Signal | **GPIO 34** | Input-only GPIO; external 10kΩ pull-up |
| **Indicators** | Buzzer (Active/Passive) | Positive (+) | **GPIO 33** | 2.3 kHz alert tone |
| **Indicators** | Red LED | Anode (+) via 220Ω | **GPIO 16** | Bowl missing / alarm status |
| **Indicators** | Green LED | Anode (+) via 220Ω | **GPIO 17** | Active feeding cycle in progress |
| **Indicators** | Blue LED | Anode (+) via 220Ω | **GPIO 23** | System standby / normal operation |

---

## 3. Power Distribution
- **Logic Power (ESP32, Sensors, LCD, RTC)**: 5V DC supply via USB / 5V rail (regulated onboard to 3.3V).
- **Motor Power (L298N & DC Agitator)**: Dedicated external 5V-9V DC power supply to L298N `VMS`/`12V` terminal to protect the ESP32 from back-EMF, inductive spikes, and voltage sags.
- **Common Ground**: All GND pins (ESP32 GND, L298N GND, External Power GND, and sensor GNDs) must be tied together.
