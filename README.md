# RTSW_5_ButtonInterrupts_Framework

A real-time embedded framework for ESP32-based systems, designed for handling button interrupts, sensor data acquisition, actuator control, and user interface via OLED display and CLI. This project is part of the Product Development initiative at Harrow, focusing on Wiedeg systems.

## Authors

- Roel Smeets
- Oliver Hofman

## Description

This framework provides a robust foundation for developing real-time applications on the ESP32 microcontroller. It integrates various hardware peripherals including analog-to-digital converters (ADC), digital-to-analog converters (DAC), I2C and SPI buses, UART communication, and interrupt-driven button inputs. The system utilizes FreeRTOS for task management, ensuring efficient multitasking and real-time performance.

Key components include:
- **Task Management**: Multiple FreeRTOS tasks for heartbeat monitoring, CLI handling, command processing, and brake control.
- **Hardware Abstraction**: Libraries for LED control, button interrupts, OLED display, EEPROM storage, and sensor interfaces.
- **Communication**: Serial CLI for user interaction and system monitoring.
- **System Testing**: Built-in test suites for hardware validation.

## Features

- **Real-Time Operation**: Utilizes FreeRTOS for concurrent task execution.
- **Interrupt Handling**: Button interrupts for responsive user input.
- **Sensor Integration**: ADC for reading sensors (angle, pressure), DAC for actuator control.
- **Display Interface**: OLED SSD1306 display for status and user feedback.
- **Data Storage**: I2C EEPROM for persistent data storage.
- **Communication Protocols**: I2C, SPI, UART support.
- **CLI Interface**: Command-line interface for system interaction and debugging.
- **System Monitoring**: Runtime statistics and task information via FreeRTOS.
- **Modular Design**: Easily extensible with additional tasks and peripherals.

## Hardware Requirements

- ESP32 development board (e.g., NodeMCU-32S)
- OLED SSD1306 display (I2C interface)
- MCP3208 12-bit ADC
- MCP4922 12-bit DAC
- Rotary encoders or sensors for angle/pressure measurement
- Push buttons for interrupts
- I2C EEPROM (e.g., 24LC256)
- Additional peripherals as per application (motors, relays, etc.)

## Software Requirements

- [PlatformIO](https://platformio.org/) IDE (recommended with VS Code)
- Arduino framework for ESP32
- FreeRTOS (included with ESP32 Arduino core)

## Dependencies

The project uses the following libraries (managed via PlatformIO):

- ESP32OLEDSSD1306 (v4.6.1)
- I2CEEPROM (v1.8.5)
- ESP32Encoder (v0.11.7)

## Installation

1. **Clone or Download**: Ensure the project is in your workspace.

2. **Open in PlatformIO**:
   - Open VS Code with PlatformIO extension installed.
   - Open the project folder containing `platformio.ini`.

3. **Build the Project**:
   - Use PlatformIO's build command or the build button in the IDE.

4. **Upload to Board**:
   - Connect your ESP32 board via USB.
   - Use PlatformIO's upload command to flash the firmware.

## Usage

1. **Power On**: Connect the ESP32 board to power. The system will initialize peripherals and start tasks.

2. **OLED Display**: The display shows system status, including version and initialization messages.

3. **Serial Monitor**: Open the serial monitor at 115200 baud to interact with the CLI.

4. **CLI Commands**: Use the command-line interface for system control and monitoring. Available commands include:
   - System status queries
   - Task information
   - Hardware testing
   - Configuration settings

5. **System Tests**: If `SYSTEMTEST_ONLY` is enabled in `Config.h`, the system runs hardware tests on startup.

## Project Structure

```
ProductDevelopment_Harrow/
├── platformio.ini              # PlatformIO configuration
├── src/                        # Source files
│   ├── RTSW_5_ButtonInterrupts_Framework.cpp  # Main application
│   ├── Config.h                 # Configuration constants
│   ├── Task*.cpp/.h             # FreeRTOS tasks
│   └── ...                      # Other source files
├── lib/                        # Custom libraries
│   ├── platformlibs/            # Hardware abstraction layers
│   ├── serialprintf/            # Serial printf utilities
│   ├── systemtests/             # Hardware test suites
│   └── utilities/               # Utility functions
├── include/                     # Header files
├── test/                        # Unit tests
└── README.md                    # This file
```

## Configuration

Key configuration options in `Config.h`:

- `SYSTEMTEST_ONLY`: Enable/disable system test mode (0 = normal operation, 1 = tests only)
- DAC/ADC parameters: Resolution, voltage ranges, channel counts
- Pin definitions: GPIO assignments for sensors and actuators

## Development Notes

- The framework is designed for modularity; add new tasks in `StartUserTasks()` function.
- Hardware initialization is handled in `platformInit()`.
- Use `SerialPrintf` for debug output instead of standard `printf`.
- FreeRTOS task priorities and stack sizes can be adjusted in task creation calls.

## Troubleshooting

- **Build Errors**: Ensure all dependencies are installed via PlatformIO.
- **I2C/OLED Issues**: Check wiring and I2C addresses.
- **Task Failures**: Monitor serial output for task creation status.
- **Interrupt Problems**: Verify button connections and interrupt pin configurations.

## Contributing

Contributions are welcome. Please follow the existing code style and add appropriate documentation.

## License

This project is open-source. Please refer to the license file if available, or contact the authors for licensing inquiries.

## Version History

- v0.1: Initial release with basic framework and hardware integration.

Last updated: May 6, 2026</content>
<parameter name="filePath">c:\Users\OllyH\Desktop\Code wiedeg\Code\ProductDevelopment_Harrow\README.md