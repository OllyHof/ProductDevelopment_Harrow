# Amiga Wiedeg Control System (ESP32)

Embedded real-time control software for the **Wiedeg (weed harrow) attachment** designed for the **Amiga agricultural robot**.  
Developed as part of the Product Development project (2025–2026) at Avans Hogeschool.

## Authors

- Roel Smeets  (Teacher)
- Oliver Hofman  (Student)

## Project Context

This software is part of a larger system that enables **mechanical weed control** using a wiedeg attached to the Amiga robot. The system allows adjustment and control of:

- Tooth pressure (tanddruk)
- Tooth angle (tandhoek)
- Working depth (via Amiga robot)
- System status and feedback

The goal is to provide a **sustainable alternative to chemical weed control** through precise mechanical soil treatment.

## System Overview

The software runs on an ESP32 and acts as the **embedded control layer** between:

1. User interface (Amiga Brain / dashboard)
2. Communication layer (CLI / future CAN-bus)
3. Actuators and sensors (motors, encoders, ADC/DAC)

Core functionality:
- Real-time actuator control
- Sensor data acquisition
- User interaction via CLI
- Modular hardware abstraction

## Key Features

- **Real-Time Operation (FreeRTOS)**  
  Multitasking for stable and predictable behavior

- **Actuator Control**  
  - PWM motor control (H-bridge)  
  - PID-based positioning (angle & pressure)

- **Sensor Integration**  
  - ADC for analog sensors  
  - Encoder feedback for positioning

- **User Interface**  
  - Serial CLI for control and debugging  
  - OLED display for status feedback

- **Interrupt Handling**  
  Responsive input via button interrupts

- **Data Storage**  
  EEPROM for persistent configuration

- **Modular Design**  
  Easily extendable system architecture

## Hardware Requirements

- ESP32 development board (e.g. NodeMCU-32S)
- MCP3208 (ADC)
- MCP4922 (DAC)
- OLED SSD1306 (I2C)
- Motor drivers (H-bridge)
- Encoders / sensors
- Push buttons (interrupt input)
- I2C EEPROM (24LC256)

## Software Architecture

The system follows a layered architecture:

### 1. Interface Layer
- CLI (Serial)
- OLED display

### 2. Control Layer
- PID controllers
- FreeRTOS task scheduling

### 3. Hardware Layer
- ADC / DAC drivers
- Communication interfaces (I2C, SPI, UART)
- Interrupt handling
- Motor control

## Project Structure

```
ProductDevelopment_Harrow/
├── platformio.ini
├── src/
│   ├── RTSW_5_ButtonInterrupts_Framework.cpp
│   ├── Config.h
│   ├── Task*.cpp/.h
├── lib/
│   ├── platformlibs/
│   ├── serialprintf/
│   ├── systemtests/
│   └── utilities/
├── include/
├── test/
└── README.md
```

## Installation

1. Open project in PlatformIO (VS Code)
2. Build the project
3. Connect ESP32 via USB
4. Upload firmware

## Usage

1. Power the ESP32
2. Open Serial Monitor (115200 baud)
3. Use CLI commands for:
   - System diagnostics
   - Actuator control
   - Sensor readings
4. Monitor system via OLED display

## Configuration

In `Config.h`:

- `SYSTEMTEST_ONLY` → enable test mode
- Pin assignments
- ADC/DAC settings
- Task configurations

## Relation to Wiedeg System

This software controls:

- Tooth angle via motor + encoder
- Tooth pressure via spring + motor control
- Feedback to the user interface
- Safe operation through interrupts and failsafes

It represents the **embedded control system of the implement**, not the full Amiga robot.

## Development Notes

- Add new features via FreeRTOS tasks
- Use `SerialPrintf` for debugging
- Tune PID controllers per mechanical setup

## Troubleshooting

- Check I2C addresses for OLED/EEPROM issues
- Verify interrupt pins for input problems
- Use CLI for debugging output

## Future Improvements

- CAN-bus integration
- Automatic depth control
- Additional sensors (force, soil feedback)
- Semi-autonomous adjustments

## License

Open-source project. Contact authors for details.

---

**Last updated:** April 2026

