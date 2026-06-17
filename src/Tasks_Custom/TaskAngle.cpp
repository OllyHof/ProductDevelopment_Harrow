///////////////////////////////////////////////////////////////////////////////
//
// TaskAngle.cpp
//
// Authors: 	Oliver Hofman
// Edit date: 	20-05-2026
//
///////////////////////////////////////////////////////////////////////////////

///////////////////////////////////////////////////////////////////////////////
// system includes

#include <Arduino.h>
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include <stdbool.h>
#include <IOLib.h>
///////////////////////////////////////////////////////////////////////////////
// hardware and task interfaces

#include <stdint.h>
#include "driver/gpio.h"
#include "Hardware_Config.h"
#include "SerialPrintf.h"

#include "TaskAngle.h"
#include "MotorUtils.h"
#include "Function_Config.h"
#include "TaskBrakes.h"
#include "TaskSleep.h"

///////////////////////////////////////////////////////////////////////////////

#define AngleToEncoder 512.0f            // Conversion factor from requested angle to encoder counts
#define Clockwise true                 // Motor direction for increasing encoder count
#define CounterClockwise false         // Motor direction for decreasing encoder count
#define ProportionalGain 1.0f          // Proportional gain for PWM output
#define IntegralGain ProportionalGain/10.0f            // Integral gain for angle control
#define DerivativeGain 3.0f*ProportionalGain       // Derivative gain for angle control

static bool CurrentDirection = Clockwise;
extern SemaphoreHandle_t xControlLoopSemaphore;

//////////////////////////////////////////////////////////////////////////////////////////////
// Angle control task
// Reads the requested angle setpoint from Machine_Settings, compares it to encoder
// feedback, and drives the motor until the target angle is reached.
void TaskAngle(void *pvParameters)
{
    int64_t encoderValue = 0;
    InitEncoder(PIN_ANGLE_SENSOR_A, PIN_ANGLE_SENSOR_B);
    encoderValue = ReadEncoder();
    float idealEncoder_float = Machine_Settings.IdealAngle * AngleToEncoder;
    int64_t idealEncoder = (int64_t)roundf(idealEncoder_float);

    float error = (float)(idealEncoder - encoderValue);
    float integral = 0.0f;
    float prevError = error;
    uint32_t lastTimeUs = micros();

    SerialPrintf("> TaskAngle start: targetAngle=%d targetEncoder=%lld currentEncoder=%lld error=%.2f\n",
                 Machine_Settings.IdealAngle, idealEncoder, encoderValue, error);

    if (fabsf(error) > ANGLE_ERROR_THRESHOLD)
    {
        if (encoderValue < idealEncoder)
        {
            ChangeDirection(PIN_ANGLE_MOTOR_DIR, CounterClockwise);
            CurrentDirection = CounterClockwise;
        }
        else
        {
            ChangeDirection(PIN_ANGLE_MOTOR_DIR, Clockwise);
            CurrentDirection = Clockwise;
        }

        SerialPrintf("> TaskAngle direction=%s\n",
                     CurrentDirection == Clockwise ? "Clockwise" : "CounterClockwise");

        // if (taskBrakes(false, PIN_BRAKE_LOWER))
        // {
        //     // Brake release failure should be handled by the calling task or error manager.
        // }

        while (fabsf(error) > ANGLE_ERROR_THRESHOLD)
        {
            uint32_t nowUs = micros();
            float dtSeconds = (float)(nowUs - lastTimeUs) * 1e-6f;
            if (dtSeconds <= 0.0f)
            {
                dtSeconds = 1e-6f;
            }
            lastTimeUs = nowUs;

            encoderValue = ReadEncoder();
            error = (float)(idealEncoder - encoderValue);

            if ((error < 0 && CurrentDirection == Clockwise) || (error > 0 && CurrentDirection == CounterClockwise))
            {
                CurrentDirection = !CurrentDirection;
                ChangeDirection(PIN_ANGLE_MOTOR_DIR, CurrentDirection);
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
            analogWrite(PIN_ANGLE_MOTOR_PWM, pwmValue);

            SerialPrintf("> TaskAngle loop: encoder=%lld error=%.2f pwm=%u dir=%s\n",
                         encoderValue, error, pwmValue,
                         CurrentDirection == Clockwise ? "Clockwise" : "CounterClockwise");

            taskSleep(100);

            }

        // if (taskBrakes(true, PIN_BRAKE_LOWER))
        // {
        //     // Brake engagement failure should be handled elsewhere.
        // }
        

    }
    
    analogWrite(PIN_ANGLE_MOTOR_PWM, 0);
    SerialPrintf("> TaskAngle complete: stopped motor and deinitialized encoder\n");
    DeinitEncoder();

    xSemaphoreGive(xControlLoopSemaphore); // Signal control loop that angle control is complete
    
    vTaskDelete(NULL);
}

void Estop_Angle()
{
    analogWrite(PIN_ANGLE_MOTOR_PWM, 0); // Stop the motor immediately
}

