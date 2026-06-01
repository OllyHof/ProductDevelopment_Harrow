# VKMPD Wiedeg Electronics Control System

## Overview

VKMPD Wiedeg Electronics is an ESP32-based embedded control system developed at Avans University of Applied Sciences for an electronically controlled agricultural harrow (Wiedeg).

After reviewing the repository structure, task documentation, PlatformIO configuration, and electrical design package, the system consists of a FreeRTOS-based control platform that manages:

- 16 hydraulic solenoids
- 8 force-adjustment actuators
- 2 angle-adjustment actuators
- Encoder feedback processing
- Emergency stop handling
- Relay-controlled power distribution
- CAN bus expansion capability
- Machine status signalling

The project is designed around modular FreeRTOS tasks and reusable platform libraries to support future expansion and hardware revisions.

---

## Hardware Architecture

### Main Controller

- ESP32-WROOM32 (NodeMCU-32S target)
- Arduino Framework
- FreeRTOS Scheduler
- PlatformIO Build Environment

### Power System

The machine is powered from a 44 VDC battery source.

| Rail | Source |
|--------|--------|
| 44 VDC | Main battery input |
| 24 VDC | Angle actuators and signal tower |
| 12 VDC (40 A) | Force actuator motor drivers |
| 12 VDC (5 A) | Solenoid bank |
| 5 VDC | ESP32, encoders, relays and communication |

### Power Converters

- DDR-60L-12
- DDR-120C-24
- DDR-15L-5
- DDR-480C-12

---

## Electrical Subsystems

### ESP32 Controller

The ESP32 acts as the central control processor and interfaces with:

- Relay Board A (K1-K16)
- Relay Board B (K17-K32)
- CAN Bus Module
- Encoder Inputs
- Motor Drivers
- Signal Tower

### Solenoid Control

Relay Board A controls sixteen solenoid outputs.

| Relays |
|----------|
| K1 – K16 |

The solenoids are powered from the dedicated 12 V supply rail.

### Force Adjustment System

Eight actuator channels are controlled through relay selection and motor driver hardware.

Features:

- Encoder feedback
- Brake control
- Direction control
- PWM speed regulation

### Angle Adjustment System

Two actuator channels provide angle positioning.

Features:

- Quadrature encoder feedback
- Index pulse support
- Bidirectional motor control

### Status Tower

Three-color machine status indication:

- Green
- Yellow
- Red
- Solid states
- Blinking states

---

## Software Architecture

The application is built around FreeRTOS tasks.

### Framework Tasks

| Task | Purpose |
|--------|----------|
| TaskHeartbeat | Runtime monitoring |
| TaskCLIHandler | Serial command interface |
| TaskCommandHandler | Command processing |

### Custom Tasks

| Task | Purpose |
|--------|----------|
| TaskBrakes | Solenoid brake control |
| TaskPressure | Force adjustment control |
| TaskAngle | Angle positioning control |
| TaskCommunicate | Communication layer (planned) |
| TaskStatusLight | Machine status indication |

---

## Control Flow

Current runtime behaviour:

1. System boot
2. Hardware initialization
3. Task startup
4. Pressure control execution
5. Angle control execution
6. Status reporting
7. Continuous monitoring

Emergency-stop functionality can interrupt all active control loops and place the machine into a safe state.

---

## Repository Structure

```text
ProductDevelopment_Harrow/
├── include/
├── lib/
│   └── platformlibs/
├── src/
│   ├── Tasks_Framework/
│   ├── Tasks_Custom/
│   ├── Config.h
│   ├── Function_Config.h
│   ├── Hardware_Config.h
│   └── RTSW_5_ButtonInterrupts_Framework.cpp
├── test/
├── platformio.ini
└── README.md
```

---

## Dependencies

### Build Environment

- PlatformIO
- Espressif32 Platform
- Arduino Framework

### External Libraries

- ESP32OLEDSSD1306
- I2CEEPROM
- ESP32Encoder

### Internal Libraries

The repository includes a large reusable platform abstraction layer providing:

- GPIO abstraction
- SPI support
- UART support
- Interrupt handling
- OLED support
- Encoder interfaces
- DAC/ADC interfaces
- Task utilities

---

## Current Implementation Status

### Implemented

- Pressure actuator control
- Angle actuator control
- Brake handling
- Encoder feedback
- Emergency stop processing
- CLI framework
- Relay control logic
- Status light mapping

### Partially Implemented

- Machine status task exists but is not started by default
- Control-loop sequencing framework exists

### Not Yet Implemented

- TaskCommunicate runtime functionality
- Active CAN communication layer
- Complete field validation and tuning

---

## Known Issues

- Communication task is currently a stub.
- Several GPIO mappings require hardware verification.
- Pressure-control gains require calibration on production hardware.
- Angle-control brake logic remains disabled in portions of the implementation.
- Signal tower may be removed if controller I/O becomes constrained.

---

## Development

Build:

```bash
pio run
```

Upload:

```bash
pio run --target upload
```

Serial Monitor:

```bash
pio device monitor
```

Target board:

```ini
board = nodemcu-32s
framework = arduino
platform = espressif32
```

---

## Safety Notice

This software was developed as part of an engineering project and should not be considered safety-certified.

Before field deployment:

- Verify all wiring
- Validate emergency-stop behaviour
- Verify actuator travel limits
- Verify encoder calibration
- Perform full hardware acceptance testing

---

## Authors

Avans University of Applied Sciences

Primary Developer:
- Oliver Hofman

Framework Support:
- Roel Smeets
