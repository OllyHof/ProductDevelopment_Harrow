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

#define AngleToEncoder 1.42f            // Conversion factor from requested angle to encoder counts (512 counts per 360° rotation)
#define Clockwise LOW                // Motor direction for increasing encoder count
#define CounterClockwise HIGH         // Motor direction for decreasing encoder count
#define ProportionalGain 1000.0f          // Proportional gain for PWM output
#define IntegralGain ProportionalGain/10.0f            // Integral gain for angle control
#define DerivativeGain ProportionalGain/5.0f       // Derivative gain for angle control

static uint8_t CurrentDirection = Clockwise;
extern SemaphoreHandle_t xControlLoopSemaphore;
int64_t encoderValue = 0;
//////////////////////////////////////////////////////////////////////////////////////////////
// Angle control task
// Reads the requested angle setpoint from Machine_Settings, compares it to encoder
// feedback, and drives the motor until the target angle is reached.
void TaskAngle(void *pvParameters)
{
    
    ResetEncoder(encoderValue); // Reset encoder to zero at the start of the task
    InitEncoder(PIN_ANGLE_SENSOR_A, PIN_ANGLE_SENSOR_B);
    encoderValue = ReadEncoder();
    float idealEncoder_float = Machine_Settings.IdealAngle * AngleToEncoder;
    int64_t idealEncoder = (int64_t)roundf(idealEncoder_float);

    float error = (float)(idealEncoder - encoderValue);
    float integral = 0.0f;
    float prevError = error;
    uint32_t lastTimeUs = micros();
    uint32_t InfolastTimeUs = micros();
    float InfodtMS = 0.0f;
    SerialPrintf("> TaskAngle start: targetAngle=%d targetEncoder=%lld currentEncoder=%lld error=%.2f\n",
                 Machine_Settings.IdealAngle, idealEncoder, encoderValue, error);

    if (fabsf(error) > ANGLE_ERROR_THRESHOLD)
    {
        if (encoderValue > idealEncoder)
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

        // if (taskBrakes(BRAKE_RELEASED, PIN_BRAKE_LOWER))
        // {
        //     if (BRAKE_FAIL_ESTOP){xSemaphoreGive(xEstopSemaphore);}
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


            float derivative = (error - prevError) / dtSeconds;
            integral += error * dtSeconds;
            float pidOutput = ProportionalGain * error + IntegralGain * integral + DerivativeGain * derivative;
            uint8_t direction = (pidOutput >= 0) ? Clockwise : CounterClockwise;

            if (direction != CurrentDirection)
            {
                ChangeDirection(PIN_ANGLE_MOTOR_DIR, direction);
                CurrentDirection = direction;
            }

            float pwmMagnitude = fabsf(pidOutput);
            if (pwmMagnitude >= 255.0f)
            {
                pwmMagnitude = 255.0f;
                integral -= error * dtSeconds; // simple anti-windup
            }
            prevError = error;

            uint8_t pwmValue = (uint8_t)LimitPWM((uint64_t)pwmMagnitude, 255, 0);
            analogWrite(PIN_ANGLE_MOTOR_PWM, pwmValue);

                ///////////////////////////////////////////////////////////////////////////////////
                // Debug commands
                if (xSemaphoreTake(xDebugSemaphore, 0) == pdTRUE)
                {
                    encoderValue = idealEncoder; // Assume ideal position for assessment
                    SerialPrintf("> TaskAngle assessment mode: encoder assumed ideal\n");
                    break; // Exit the control loop for assessment
                    
                }

                if (motorinfoEnabled)
                {
                    uint32_t InfonowUs = micros();
                    InfodtMS = (float)(InfonowUs - InfolastTimeUs) * 1e-3f;

                    if (InfodtMS >= MAX_MESSAGE_RATE_MS)
                    {
                        SerialPrintf("> TaskAngleloop encoder=%lld error=%.2f pwm=%u dir=%s\n",
                                    encoderValue,
                                    error,
                                    pwmValue,
                                    CurrentDirection == Clockwise ? "Clockwise" : "CounterClockwise");

                        InfolastTimeUs = InfonowUs;
                    }
                }
                ///////////////////////////////////////////////////////////////////////////////////
        }
        
        // if (taskBrakes(BRAKE_ENGAGED, PIN_BRAKE_LOWER))
        // {
        //     if (BRAKE_FAIL_ESTOP){xSemaphoreGive(xEstopSemaphore);}
        // }
        

    }
    
    integral = 0.0f;
    prevError = 0.0f;
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

