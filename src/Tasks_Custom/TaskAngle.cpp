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

#include "TaskAngle.h"
#include "MotorUtils.h"
#include "Function_Config.h"
#include "TaskBrakes.h"
#include "TaskSleep.h"

///////////////////////////////////////////////////////////////////////////////

#define AngleToEncoder 1.0f            // Conversion factor from requested angle to encoder counts
#define Clockwise true                 // Motor direction for increasing encoder count
#define CounterClockwise false         // Motor direction for decreasing encoder count
#define ProportionalGain 1.0f          // Proportional gain for PWM output
#define ErrorThreshold 5               // Minimum encoder error before motor activation

static bool CurrentDirection = Clockwise;
extern SemaphoreHandle_t xControlLoopSemaphore;

//////////////////////////////////////////////////////////////////////////////////////////////
// Angle control task
// Reads the requested angle setpoint from Machine_Settings, compares it to encoder
// feedback, and drives the motor until the target angle is reached.
void TaskAngle(void *pvParameters)
{
    uint64_t encoderValue = 0;
    uint64_t idealEncoder = Machine_Settings.Idealangle * AngleToEncoder;

    float error = (float)(idealEncoder - encoderValue);
    
    if (abs(error) > ErrorThreshold)
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

        if (taskBrakes(false, PIN_BRAKE_LOWER))
        {
            // Brake release failure should be handled by the calling task or error manager.
        }

        while (true)
        {
            encoderValue = ReadEncoder(encoderValue, PIN_ANGLE_SENSOR_A, PIN_ANGLE_SENSOR_B);
            idealEncoder = Machine_Settings.Idealangle * AngleToEncoder;
            error = (float)(idealEncoder - encoderValue);

            if (abs(error) <= ErrorThreshold)
            {
                break;
            }

            if ((error < 0 && CurrentDirection == Clockwise) || (error > 0 && CurrentDirection == CounterClockwise))
            {
                CurrentDirection = !CurrentDirection;
                ChangeDirection(PIN_ANGLE_MOTOR_DIR, CurrentDirection);
            }

            uint8_t pwmValue = LimitPWM((uint64_t)((abs(error) * 255.0f) / idealEncoder), 255, 0);
            io_SetBit_Analog(PIN_ANGLE_MOTOR_PWM, pwmValue);

            taskSleep(10);
        }

        if (taskBrakes(true, PIN_BRAKE_LOWER))
        {
            // Brake engagement failure should be handled elsewhere.
        }

        io_SetBit_Analog(PIN_ANGLE_MOTOR_PWM, 0);
    }

    xSemaphoreGive(xControlLoopSemaphore); // Signal control loop that angle control is complete
    
    while (true)
    {
        vTaskDelay(portMAX_DELAY); // Suspend the task indefinitely after completing control
    }

    taskSleep(10);
}

