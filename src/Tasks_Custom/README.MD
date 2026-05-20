## Author
Oliver Hofman

## Tasks
The 5 main software tasks are:
* TaskCommunicate
* TaskBrakes
* TaskStatusLight
* TaskPressure
* TaskAngle

These are described within this README file and their respective .cpp and .h files.

## TaskCommunicate
Provides communication-related FreeRTOS tasks for sending and receiving machine settings.

Key behavior:
* `TaskCommunicate_Send` is a FreeRTOS task that periodically runs and is intended to send data over the communication interface.
* `TaskCommunicate_Receive` is a FreeRTOS task that periodically updates `Machine_Settings`, including `Idealangle` and `IdealPressure`.
* Current implementation uses placeholder values and a 10 ms task delay.

## TaskBrakes
Handles brake engagement and disengagement through a simple pin control interface.

Key behavior:
* `taskBrakes(bool BrakeOn, uint8_t BrakePin)` sets the specified brake pin state using `io_SetBit`.
* When `BrakeOn` is true, the function engages the brake pin; when false, it disengages the brake pin.
* The function currently returns `false` for both branches, so error handling is present in the signature but not active in the current stub implementation.

## TaskStatusLight
Manages the status tower LEDs using a status code lookup table.

Key behavior:
* `taskStatusLight(uint8_t Status)` maps a status code to LED outputs: red, green, yellow, and blink.
* It supports 10 status codes:
  * `STATUS_NOCONTROL` - yellow blinking
  * `STATUS_ERROR_HARD` - red blinking
  * `STATUS_ERROR_SOFT` - red solid
  * `STATUS_ALLGOOD` - green solid
  * `STATUS_STANDBY` - yellow solid
  * `STATUS_RUNNING` - green blinking
  * `STATUS_WARNING` - red + yellow solid
  * `STATUS_DISABLED` - all LEDs off
  * `STATUS_MAINTENANCE` - all colors blinking
  * `STATUS_WORKING` - green + yellow blinking
* For hardware builds, it writes to `PIN_TREE_RED`, `PIN_TREE_GREEN`, `PIN_TREE_YELLOW`, and `PIN_TREE_BLINK`.
* For non-hardware builds, it prints the status values to serial for testing.

## TaskPressure
Controls pressure motor and brake coordination based on machine settings.

Key behavior:
* `TaskPressure(void *pvParameters)` loops through a motor configuration array and adjusts motor/brake outputs toward `Machine_Settings.IdealPressure`.
* Uses `LimitPWM` to clamp PWM values, `ChangeDirection` to set motor direction, and `ReadEncoder` to update encoder feedback.
* Coordinates with `taskBrakes` to release and engage brakes during control operations.
* Current implementation includes example conversion logic, error threshold checks, and placeholder encoder feedback.

## TaskAngle
Present in the current source tree, but does not yet implement a dedicated task function or application logic.

Key status:
* `TaskAngle.cpp` and `TaskAngle.h` exist.
* The module currently includes header and source scaffolding, but no active task implementation is defined at this time.

## Usage

These modules implement machine-specific FreeRTOS tasks and helpers. Tasks are created by the framework in `RTSW_5_ButtonInterrupts_Framework.cpp` (see `StartUserTasks()` and `TaskControlLoop()`). Typical runtime flow:

- `TaskCommunicate_Receive` — receives UART/CAN commands and updates `Machine_Settings`.
- `TaskCommunicate_Send` — periodically sends STATUS lines (angle, pressure).
- `TaskPressure` — executes the pressure control loop for each configured pressure channel.
- `TaskAngle` — performs angle control when created by the control loop.
- `MachineStatus` / `taskStatusLight` — updates status tower LEDs.
- `taskBrakes()` — helper to engage/disengage brakes for a given brake pin.

Entry points and task names (used for `info_RegisterTaskByName` and task creation):

- `task_Heartbeat` (task_heartbeat)
- `task_CLIHandler` (task_cli)
- `task_CommandHandler` (task_cmd)
- `TaskCommunicate_Receive` (task_communicate_rx)
- `TaskCommunicate_Send` (task_communicate_tx)
- `TaskControlLoop` (task_control_loop)
- `MachineStatus` (task_status)

## Configuration

- Set machine setpoints via the communication interface (SET ANGLE=<value>,PRESSURE=<value>). The `Machine_Settings` struct is declared in `TaskCommunicate.h`.
- Pin and hardware configuration is controlled from `Hardware_Config.h` and function/feature flags from `Function_Config.h` and `Config.h`.

## Testing

- Unit/integration tests should be added under `test/` using PlatformIO Test Runner. Current test README contains general guidance but no project-specific tests.
- Recommended tests:
  - `taskBrakes` behavior with mocked `io_SetBit` and `digitalRead`.
  - `TaskCommunicate` parsing for `SET`/`GET`/`PING` commands.
  - `MotorUtils::LimitPWM`, direction control and `ReadEncoder` behavior.

## Known Issues & TODOs

- `TaskAngle`:
  - Implementation present but needs review for encoder conversion and continuous task operation. Ensure `AngleToEncoder` conversion and PWM calculation are safe for zero setpoints.
- `taskBrakes`:
  - Previously returned `false` unconditionally; implementation fixed to set the pin and return success. Verify physical brake polarity and add readback-based error handling when hardware mapping is final.
- `TaskStatusLight`:
  - `hasFatalError()` and `hasSoftError()` are placeholders and need actual error detection logic.
- Hardware pin assignments in `Hardware_Config.h` are placeholders — update to match Harrow wiring before hardware integration.

## Contributing

- If you add/modify tasks, update this README with behavior, task names and expected timing/priority.



