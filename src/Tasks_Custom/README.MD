## Author
Oliver Hofman

## Tasks
The 5 main custom task modules are:
* TaskCommunicate
* TaskBrakes
* TaskStatusLight
* TaskPressure
* TaskAngle

This README describes the current implementation state of each module and how they are intended to work together.

## TaskCommunicate
This module is currently a stub.

Key details:
* `TaskCommunicate.h` and `TaskCommunicate.cpp` exist, but the communication task functions are not implemented in the current source.
* The framework currently has the communication task creation commented out in `RTSW_5_ButtonInterrupts_Framework.cpp`.
* The current project uses the CLI command interface instead of active CAN/UART communication tasks.

## TaskBrakes
Implements brake control using GPIO pin writes and simple verification.

Key behavior:
* `taskBrakes(bool BrakeOn, uint8_t BrakePin)` writes the requested logical state to the brake pin using `io_SetBit`.
* It verifies the written state with `digitalRead()` and returns `false` on success, `true` on failure.
* `Estop_Brake()` iterates all configured brakes and attempts to disengage them during an emergency stop.

## TaskStatusLight
Manages machine status indication through LED outputs and status code mapping.

Key behavior:
* `taskStatusLight(uint8_t Status)` translates a status code into `PIN_TREE_RED`, `PIN_TREE_GREEN`, `PIN_TREE_YELLOW`, and `PIN_TREE_BLINK` outputs when `HARDWARE_CONNECTED == HARDWARE_HARROW`.
* In non-hardware builds, it prints the resolved LED pattern to serial for diagnostics.
* `MachineStatus(void *pvParameters)` exists and determines status from task states, but the framework currently does not create this task by default.

Supported status codes:
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

## TaskPressure
Implements the pressure control loop for the machine.

Key behavior:
* `TaskPressure(void *pvParameters)` iterates configured pressure channels and adjusts motor outputs toward `Machine_Settings.IdealPressure`.
* It uses `LimitPWM()` for PWM scaling, `ChangeDirection()` for motor direction, and `ReadEncoder()` to sample encoder feedback.
* Brakes are released before motion and re-engaged after the setpoint is reached.
* Once each pressure channel reaches its target, the task signals `xControlLoopSemaphore` and suspends indefinitely.

## TaskAngle
Implements the angle control step for the machine.

Key behavior:
* `TaskAngle(void *pvParameters)` reads `Machine_Settings.Idealangle` and drives the angle motor until the encoder reaches the target.
* It applies proportional PWM and switches direction when the error crosses zero.
* Lower brake handling is present in comments but currently disabled.
* After reaching the setpoint, the task signals `xControlLoopSemaphore` and suspends indefinitely.

## Current Runtime Integration

The active startup path is defined in `RTSW_5_ButtonInterrupts_Framework.cpp`.

Current task startup behavior:
* `StartUserTasks()` creates `TestTask` and `ESTOPHandler`.
* The communication tasks and `MachineStatus` are currently commented out and not started by default.
* `TaskControlLoop()` exists and, when created, starts `TaskPressure` followed by `TaskAngle` in sequence using `xControlLoopSemaphore`.
* `ESTOPHandler()` handles emergency stop events, stops motors, disables brakes, and can restart `TaskControlLoop` after reset.

## Configuration

* `Machine_Settings` is referenced throughout the control tasks, but its definition is not in this directory.
* Pin definitions and hardware configuration are controlled from `Hardware_Config.h`.
* Compile-time feature flags live in `Function_Config.h` and `Config.h`.

## Testing

* Add unit/integration tests under `test/` using the PlatformIO Test Runner.
* Existing code does not include project-specific tests in the repository.
* Useful targets for tests:
  * `TaskBrakes` brake pin logic and error reporting
  * `TaskPressure` PWM scaling and encoder feedback handling
  * `TaskAngle` direction switching and setpoint stabilization
  * `TaskStatusLight` status-to-LED mapping

## Known Issues & TODOs

* `TaskCommunicate` is not implemented and the communication task creation is disabled.
* `TaskStatusLight::MachineStatus()` is present but not currently launched by the default task startup path.
* `TaskAngle` still has brake control disabled in comments and needs validation for real hardware use.
* `TaskPressure` assumes encoder feedback and gain/mapping values that require tuning for the actual pressure system.
* Verify all hardware pin assignments in `Hardware_Config.h` against the final Harrow wiring.

## Contributing

* If you add or modify tasks, update this README with the expected control flow, task creation state, and hardware coverage.



