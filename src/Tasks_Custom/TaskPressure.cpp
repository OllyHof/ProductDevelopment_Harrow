///////////////////////////////////////////////////////////////////////////////
//
// TaskPressure.cpp
//
// Authors: 	Oliver Hofman
// Edit date: 	19-05-2026
//
// This module implements the pressure control task for the machine. It reads
// the requested pressure setpoint from Machine_Settings, compares it to the
// encoder feedback, and operates the motor, direction control, and brakes until
// the target pressure is achieved.
//
///////////////////////////////////////////////////////////////////////////////

///////////////////////////////////////////////////////////////////////////////
// Platform and FreeRTOS includes

#include <Arduino.h>
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include <stdbool.h>
#include <IOLib.h>
///////////////////////////////////////////////////////////////////////////////
// Hardware and task interfaces

#include <stdint.h>
#include "driver/gpio.h"
#include "Hardware_Config.h"

#include "TaskPressure.h"
#include "MotorUtils.h"
#include "Function_Config.h"
#include "TaskBrakes.h"
#include "TaskSleep.h"
///////////////////////////////////////////////////////////////////////////////

extern SemaphoreHandle_t xControlLoopSemaphore;

typedef struct
{
    gpio_num_t MotorID;     // Motor select GPIO for the pressure channel
    gpio_num_t BrakeID;     // Brake control GPIO for the pressure channel
    int EncoderValue;       // Latest encoder count used for closed-loop control
} MotorConfig_t;

MotorConfig_t motorConfigs[] = 
{
    {PIN_PRESSURE_MOTOR_SEL_1, PIN_BRAKE_UPPER_1, 0}, // Pressure channel 1
    {PIN_PRESSURE_MOTOR_SEL_2, PIN_BRAKE_UPPER_2, 0}, // Pressure channel 2
    {PIN_PRESSURE_MOTOR_SEL_3, PIN_BRAKE_UPPER_3, 0}, // Pressure channel 3
    {PIN_PRESSURE_MOTOR_SEL_4, PIN_BRAKE_UPPER_4, 0}, // Pressure channel 4
};

#define PressureToEncoder 1.0f            // Conversion factor from requested pressure to encoder counts
#define Clockwise true                   // Motor direction used for Positive pressure adjustment direction
#define CounterClockwise false           // Motor direction used for Negative pressure adjustment direction
#define Error (float)(idealEncoder - motorConfigs[i].EncoderValue)
#define MotorPWM (uint8_t)(LimitPWM((abs(ProportionalGain * Error) * 255) / idealEncoder, 255, 0))
#define ProportionalGain 1.0f            // Proportional control gain for PWM output
#define ErrorThreshold 100               // Minimum encoder error before the motor is enabled
#define ErrorTooHigh (abs(Error) > ErrorThreshold)

bool CurrentDirection = Clockwise;
bool* CurrentDirectionPtr = &CurrentDirection; // Shared direction state used by ChangeDirection
//////////////////////////////////////////////////////////////////////////////////////////////
// Pressure control task
// Iterates the list of pressure channels, applies the control loop to each one,
// and manages motor PWM, direction, and brakes until the encoder reading matches
// the requested pressure target.
void TaskPressure(void *pvParameters)
{
    for (int i = 0; i < sizeof(motorConfigs) / sizeof(MotorConfig_t); i++)
    {
        MotorConfig_t *config = &motorConfigs[i];
        uint64_t idealEncoder = Machine_Settings.IdealPressure * PressureToEncoder; // Setpoint in encoder counts

        if (ErrorTooHigh)
        {
            // Enable the selected pressure motor output
            io_SetBit(config->MotorID, true);

            if (config->EncoderValue > idealEncoder)
            {
                // Current encoder count is above the setpoint; choose one direction
                ChangeDirection(PIN_PRESSURE_MOTOR_DIR, Clockwise);
                CurrentDirection = Clockwise;
            }
            else
            {
                // Current encoder count is below the setpoint; choose opposite direction
                ChangeDirection(PIN_PRESSURE_MOTOR_DIR, CounterClockwise);
                CurrentDirection = CounterClockwise;
            }

            if (taskBrakes(false, config->BrakeID))
            {
                // Brake release failure should be handled by the calling task or error manager
            }

            while (ErrorTooHigh)
            {
                // Drive the motor with PWM proportional to the remaining error
                io_SetBit_Analog(PIN_PRESSURE_MOTOR_PWM, MotorPWM);

                while (ErrorTooHigh)
                {
                    // Sample encoder feedback while the motor is active
                    config->EncoderValue = ReadEncoder(config->EncoderValue, PIN_PRESSURE_SENSOR_A, PIN_PRESSURE_SENSOR_B);
                    if (Error < 0)
                    {
                        // Reverse direction if the measured value has crossed the setpoint
                        CurrentDirection = !CurrentDirection;
                        ChangeDirection(PIN_PRESSURE_MOTOR_DIR, CurrentDirection);
                        break;
                    }
                }

                taskSleep(10); // Yield to other tasks and allow sensor settling
            }

            // Engage the brake after the setpoint has been reached
            if (taskBrakes(true, config->BrakeID))
            {
                // Brake engagement failure should be handled elsewhere
            }

            // Stop the motor once pressure control is complete
            io_SetBit_Analog(PIN_PRESSURE_MOTOR_PWM, 0);
        }
    }

    xSemaphoreGive(xControlLoopSemaphore); // Signal control loop that angle control is complete

    while (true)
    {
        vTaskDelay(portMAX_DELAY); // Suspend the task indefinitely after completing control
    }
}
