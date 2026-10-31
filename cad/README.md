# CAD Models & Mechanical Design

This directory contains the 3D computer-aided design (CAD) files, 3D printing mesh files (`.STL`), and interchange exchange formats (`.STEP`) for the **Automatic Turtle Feeder**.

---

## 📐 Expected 3D Printed Components
1. **Pellet Hopper (`hopper.stl` / `hopper.step`)**:
   - Funnel-shaped reservoir holding commercial dry turtle pellets.
   - Sized for several days of autonomous feeding with moisture-shielding lid.
2. **Dispensing Gate & Chute (`dispensing_gate.stl`)**:
   - Actuated directly by the **SG90 Servo** horn (`0°` closed, `35°` open).
   - Designed with close tolerances to prevent pellet crushing while sealing the chute when idle.
3. **Internal Agitator / Jiggle Impeller (`agitator_blade.stl`)**:
   - Mounted onto the DC motor shaft driven by the **L298N**.
   - Oscillates back and forth inside the lower hopper throat to break pellet arching/bridging.
4. **Food Bowl Alignment Base (`bowl_mount.stl`)**:
   - Locates the feeding bowl directly under the chute.
   - Integrates a mounting slot for the **IR sensor** to detect bowl placement.
5. **Electronics Chassis & Sensor Brackets (`enclosure.stl`)**:
   - Holds the ESP32 Dev1, L298N module, 16x2 LCD display, RTC, and ultrasonic sensor overhead.

---

## 🖨️ Recommended 3D Printing Settings
- **Material**: PLA or PETG (PETG recommended for higher moisture resistance near aquariums).
- **Layer Height**: 0.20 mm.
- **Infill**: 20% - 30% Gyroid or Grid.
- **Supports**: Only required for overhead overhangs on the chute mount.
