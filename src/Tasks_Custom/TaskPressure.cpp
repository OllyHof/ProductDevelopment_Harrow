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
#include "SerialPrintf.h"

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
    int64_t EncoderValue;   // Latest encoder count used for closed-loop control
} MotorConfig_t;

MotorConfig_t motorConfigs[] = 
{
    {PIN_PRESSURE_MOTOR_SEL_1, PIN_BRAKE_UPPER_1, 0}, // Pressure channel 1
    /* Section commented out since it won't be used in the current machine configuration
    {PIN_PRESSURE_MOTOR_SEL_2, PIN_BRAKE_UPPER_2, 0}, // Pressure channel 2
    {PIN_PRESSURE_MOTOR_SEL_3, PIN_BRAKE_UPPER_3, 0}, // Pressure channel 3
    {PIN_PRESSURE_MOTOR_SEL_4, PIN_BRAKE_UPPER_4, 0}, // Pressure channel 4
    */
};

#define PressureToEncoder 1000.2f            // Conversion factor from requested pressure to encoder counts
#define Clockwise HIGH                   // Motor direction used for Positive pressure adjustment direction
#define CounterClockwise LOW           // Motor direction used for Negative pressure adjustment direction
#define ProportionalGain 5.0f          // Proportional gain for PWM output
#define IntegralGain ProportionalGain/10.0f            // Integral gain for angle control
#define DerivativeGain 3.0f*ProportionalGain       // Derivative gain for angle control

uint8_t CurrentDirection = Clockwise;
uint8_t* CurrentDirectionPtr = &CurrentDirection; // Shared direction state used by ChangeDirection

//////////////////////////////////////////////////////////////////////////////////////////////
// Pressure control task
// Iterates the list of pressure channels, applies the control loop to each one,
// and manages motor PWM, direction, and brakes until the encoder reading matches
// the requested pressure target.
void TaskPressure(void *pvParameters)
{
    InitEncoder(PIN_PRESSURE_SENSOR_A, PIN_PRESSURE_SENSOR_B);

    for (int i = 0; i < sizeof(motorConfigs) / sizeof(MotorConfig_t); i++)
    {
        MotorConfig_t *config = &motorConfigs[i];
        float idealEncoder_float = Machine_Settings.IdealPressure * PressureToEncoder;
        int64_t idealEncoder = (int64_t)roundf(idealEncoder_float);
        ResetEncoder(config->EncoderValue);
        config->EncoderValue = ReadEncoder();

        float error = (float)(idealEncoder - config->EncoderValue);
        float integral = 0.0f;
        float prevError = error;
        uint32_t lastTimeUs = micros();

        SerialPrintf("> TaskPressure channel=%d start targetPressure=%.2f targetEncoder=%lld currentEncoder=%lld error=%.2f\n",
                     i + 1, Machine_Settings.IdealPressure, idealEncoder, config->EncoderValue, error);

        if (fabsf(error) > PRESSURE_ERROR_THRESHOLD)
        {
            // Enable the selected pressure motor output
            digitalWrite(config->MotorID, LOW);

            // Determine initial direction based on setpoint vs current value
            if (config->EncoderValue > idealEncoder)
            {
                ChangeDirection(PIN_PRESSURE_MOTOR_DIR, Clockwise);
                CurrentDirection = Clockwise;
            }
            else
            {
                ChangeDirection(PIN_PRESSURE_MOTOR_DIR, CounterClockwise);
                CurrentDirection = CounterClockwise;
            }

            SerialPrintf("> TaskPressure channel=%d initial direction=%s\n",
                         i + 1,
                         CurrentDirection == Clockwise ? "Clockwise" : "CounterClockwise");

            if (taskBrakes(false, config->BrakeID))
            {
                // Brake release failure should be handled by the calling task or error manager
            }

            // Single control loop - update every iteration
            while (true)
            {
                uint32_t nowUs = micros();
                float dtSeconds = (float)(nowUs - lastTimeUs) * 1e-6f;
                if (dtSeconds <= 0.0f)
                {
                    dtSeconds = 1e-6f;
                }
                lastTimeUs = nowUs;

                // Sample encoder feedback
                config->EncoderValue = ReadEncoder();
                error = (float)(idealEncoder - config->EncoderValue);

                // Check if error crossed zero; reverse direction if needed
                if ((error < 0 && CurrentDirection == Clockwise) ||
                    (error > 0 && CurrentDirection == CounterClockwise))
                {
                    CurrentDirection = !CurrentDirection;
                    ChangeDirection(PIN_PRESSURE_MOTOR_DIR, CurrentDirection);
                }

                float derivative = (error - prevError) / dtSeconds;
                integral += error * dtSeconds;
                float pidOutput = ProportionalGain * error + IntegralGain * integral + DerivativeGain * derivative;
                float pwmMagnitude = fabsf(pidOutput);
                if (pwmMagnitude > 255.0f)
                {
                    pwmMagnitude = 255.0f;
                    integral -= error * dtSeconds; // simple anti-windup
                }
                prevError = error;

                uint8_t pwmValue = (uint8_t)LimitPWM((uint64_t)pwmMagnitude, 255, 0);
                analogWrite(PIN_PRESSURE_MOTOR_PWM, pwmValue);
/*
                SerialPrintf("> TaskPressure channel=%d loop encoder=%lld error=%.2f pwm=%u dir=%s\n",
                             i + 1,
                             config->EncoderValue,
                             error,
                             pwmValue,
                             CurrentDirection == Clockwise ? "Clockwise" : "CounterClockwise");

                taskSleep(100); // Yield to other tasks and allow sensor settling
*/              if (fabsf(error) <= PRESSURE_ERROR_THRESHOLD)
                {
                    SerialPrintf("> TaskPressure channel=%d target reached: encoder=%lld error=%.2f pwm=%u dir=%s\n",
                                 i + 1,
                                 config->EncoderValue,
                                 error,
                                 pwmValue,
                                 CurrentDirection == Clockwise ? "Clockwise" : "CounterClockwise");
                    break; // Exit the control loop when target is reached
                }
            }

            // Engage the brake after the setpoint has been reached
            if (taskBrakes(true, config->BrakeID))
            {
                // Brake engagement failure should be handled elsewhere
            }

            // Stop the motor once pressure control is complete
            analogWrite(PIN_PRESSURE_MOTOR_PWM, 0);
            SerialPrintf("> TaskPressure channel=%d complete: motor stopped and brake engaged\n", i + 1);
            DeinitEncoder();
        }
    }

    xSemaphoreGive(xControlLoopSemaphore); // Signal control loop that pressure control is complete

    vTaskDelete(NULL); // Cleanly delete the task after completion
}


void Estop_Pressure()
{
    for(int i = 0; i < sizeof(motorConfigs) / sizeof(MotorConfig_t); i++)
    {
        MotorConfig_t *config = &motorConfigs[i];
        analogWrite(PIN_PRESSURE_MOTOR_PWM, 0); // Stop the motor immediately
    }
}