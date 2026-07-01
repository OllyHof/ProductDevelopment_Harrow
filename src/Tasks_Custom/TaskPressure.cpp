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
#include "freertos/semphr.h"
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
#include "BrakeLib.h"
#include "TaskSleep.h"
///////////////////////////////////////////////////////////////////////////////

extern SemaphoreHandle_t xControlLoopSemaphore;

MotorConfig_t motorConfigs[] = 
{
    {PIN_PRESSURE_MOTOR_SEL_1, PIN_BRAKE_UPPER_1, 0,}, // Pressure channel 1
    /* Section commented out since it won't be used in the current machine configuration
    {PIN_PRESSURE_MOTOR_SEL_2, PIN_BRAKE_UPPER_2, 0,}, // Pressure channel 2
    {PIN_PRESSURE_MOTOR_SEL_3, PIN_BRAKE_UPPER_3, 0,}, // Pressure channel 3
    {PIN_PRESSURE_MOTOR_SEL_4, PIN_BRAKE_UPPER_4, 0,}, // Pressure channel 4
    */
};

#define NUM_MOTOR_CONFIGS (sizeof(motorConfigs) / sizeof(MotorConfig_t))

#define PressureToEncoder 1067.4f           // Conversion factor from requested pressure to encoder counts 
// 2.4 input = 0.5 output shaft rotation = 2562 encoder counts
// => 1 input unit = 1067 counts
#define Clockwise HIGH                   // Motor direction used for Positive pressure adjustment direction
#define CounterClockwise LOW          // Motor direction used for Negative pressure adjustment direction
#define ProportionalGain 0.0010f          // Proportional gain for PWM output
#define IntegralGain ProportionalGain/10000.0f            // Integral gain for angle control
#define DerivativeGain ProportionalGain*50000.0f       // Derivative gain for angle control

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

    for (int i = 0; i < NUM_MOTOR_CONFIGS; i++)
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
        uint32_t InfolastTimeUs = micros();
        float InfodtMS = 0.0f;

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

            if (Brake_Set(config->BrakeID, BRAKE_RELEASED)){if(BRAKE_FAIL_ESTOP){xSemaphoreGive(xEstopSemaphore);}}

            // Single control loop - update every iteration
            uint32_t softStartStartUs = micros();
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
                float derivative = (error - prevError) / dtSeconds;
                integral += error * dtSeconds;
                float pidOutput = ProportionalGain * error + IntegralGain * integral + DerivativeGain * derivative;
                uint8_t direction = (error >= 0) ? Clockwise : CounterClockwise;

                if (direction != CurrentDirection)
                {
                    ChangeDirection(PIN_PRESSURE_MOTOR_DIR, direction);
                    CurrentDirection = direction;
                }

                int pwmMagnitude = roundf(fabsf(pidOutput));// * softStartGain;
                if (pwmMagnitude >= AnalogWriteMaxValue)
                {
                    integral -= error * dtSeconds;
                    integral = constrain(integral, -10000, 10000);
                    pwmMagnitude = AnalogWriteMaxValue;
                }   

                prevError = error;

                analogWrite(PIN_PRESSURE_MOTOR_PWM, pwmMagnitude);

                if (fabsf(error) <= PRESSURE_ERROR_THRESHOLD)
                {
                    SerialPrintf("> TaskPressure channel=%d target reached: encoder=%lld error=%.2f pwm=%u dir=%s\n",
                                 i + 1,
                                 config->EncoderValue,
                                 error,
                                 pwmMagnitude,
                                 CurrentDirection == Clockwise ? "Clockwise" : "CounterClockwise");
                    break; // Exit the control loop when target is reached
                }
                ///////////////////////////////////////////////////////////////////////////////////
                // Debug commands
                if (xSemaphoreTake(xDebugSemaphore, 0) == pdTRUE)
                {
                    config->EncoderValue = idealEncoder; // Assume ideal position for assessment
                    SerialPrintf("> TaskPressure channel=%d assessment mode: encoder assumed ideal\n", i + 1);
                    break; // Exit the control loop for assessment
                    
                }

                if (motorinfoEnabled)
                {
                    uint32_t InfonowUs = micros();
                    InfodtMS = (float)(InfonowUs - InfolastTimeUs) * 1e-3f;

                    if (InfodtMS >= MAX_MESSAGE_RATE_MS)
                    {
                        SerialPrintf("> TaskPressure channel=%d loop encoder=%lld error=%.2f pwm=%u dir=%s\n",
                                    i + 1,
                                    config->EncoderValue,
                                    error,
                                    pwmMagnitude,
                                    CurrentDirection == Clockwise ? "Clockwise" : "CounterClockwise");

                        InfolastTimeUs = InfonowUs;
                    }
                }
                ///////////////////////////////////////////////////////////////////////////////////
            }

            // Engage the brake after the setpoint has been reached
            if (Brake_Set(config->BrakeID, BRAKE_ENGAGED)){if(BRAKE_FAIL_ESTOP){xSemaphoreGive(xEstopSemaphore);}}
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
    for(int i = 0; i < NUM_MOTOR_CONFIGS; i++)
    {
        analogWrite(PIN_PRESSURE_MOTOR_PWM, 0); // Stop the motor immediately
    }
}

