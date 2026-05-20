///////////////////////////////////////////////////////////////////////////////
//
// TaskPressure.cpp
//
// Authors: 	Oliver Hofman
// Edit date: 	19-05-2026
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
// application includes

#include <stdint.h>
#include "driver/gpio.h"
#include "Hardware_Config.h"

#include "TaskPressure.h"
#include "TaskCommunicate.h"
#include "TaskBrakes.h"
#include "TaskSleep.h"
///////////////////////////////////////////////////////////////////////////////

typedef struct
{
    gpio_num_t MotorID; 
    gpio_num_t BrakeID; 
    int EncoderValue; 
} MotorConfig_t;

MotorConfig_t motorConfigs[] = 
{
    {PIN_PRESSURE_MOTOR_SEL_1, PIN_BRAKE_UPPER_1, 0}, // Motor 1
    {PIN_PRESSURE_MOTOR_SEL_2, PIN_BRAKE_UPPER_2, 0}, // Motor 2
    {PIN_PRESSURE_MOTOR_SEL_3, PIN_BRAKE_UPPER_3, 0}, // Motor 3
    {PIN_PRESSURE_MOTOR_SEL_4, PIN_BRAKE_UPPER_4, 0}, // Motor 4
};

#define PressureToEncoder 1.0 // Example conversion factor from pressure to encoder value
#define Clockwise true
#define CounterClockwise false
#define Error (float)(idealEncoder - motorConfigs[i].EncoderValue) // Example error condition for brake engagement
#define MotorPWM (uint8_t)(LimitPWM((abs(ProportionalGain * Error)*255)/idealEncoder, 255, 0)) // Example motor PWM calculation based on error
#define ProportionalGain 1.0 // Example proportional gain for motor control
#define ErrorThreshold 100 // Example error threshold for acceptable range
#define ErrorTooHigh (abs(Error) > ErrorThreshold)

bool CurrentDirection = Clockwise;
bool* CurrentDirectionPtr = &CurrentDirection; // Pointer to track current direction for brake control
//////////////////////////////////////////////////////////////////////////////////////////////
// void TaskPressure(void *pvParameters);
// This task controls the pressure motors based on the desired pressure and angle settings.
void TaskPressure(void *pvParameters)
{
        // Control logic for pressure motors based on Machine_Settings
        for (int i = 0; i < sizeof(motorConfigs) / sizeof(MotorConfig_t); i++)
        {
            MotorConfig_t *config = &motorConfigs[i];
            
            
            uint64_t idealEncoder = Machine_Settings.IdealPressure * PressureToEncoder; // Get the ideal pressure from the communication data
            
            if (ErrorTooHigh) { // Example condition to engage brakes
                io_SetBit(config->MotorID, true); // Example: Activate motor based on settings
                
                if (config->EncoderValue > idealEncoder) { // Example condition to engage brakes
                    ChangeDirection(Clockwise); // Example: Change direction to wind up pressure
                }
                else{ // Example condition to release pressure
                    ChangeDirection(CounterClockwise); // Example: Change direction to release pressure
                }

                if (taskBrakes(false, config->BrakeID)){
                    // Give Interrupt Error, brake error
                }

                while (ErrorTooHigh) { // Example condition to maintain motor control until desired pressure is reached
                    io_SetBit_Analog(PIN_PRESSURE_MOTOR_PWM, MotorPWM); // Example: Set motor PWM based on error
                    while (ErrorTooHigh) { // Example condition to check if error is within acceptable range
                        config->EncoderValue = ReadEncoder(config->EncoderValue, PIN_PRESSURE_SENSOR_A, PIN_PRESSURE_SENSOR_B); // Example: Read encoder value for feedback
                        if (Error < 0)
                        {
                            ChangeDirection(!CurrentDirection);
                            break;
                        }
                    }
                    taskSleep(10); // Delay for 10 milliseconds
                }
                
                if(taskBrakes(true, config->BrakeID)){
                    // Give Interrupt Error, brake error
                }

                io_SetBit_Analog(PIN_PRESSURE_MOTOR_PWM, 0); // Example: Deactivate motor after reaching desired pressure
            }
           
        }    
        taskSleep(10); // Delay for 10 milliseconds
}

///////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

void ChangeDirection(bool direction)
{
    *CurrentDirectionPtr = direction; // Update the current direction
    io_SetBit(PIN_PRESSURE_MOTOR_DIR, direction); // Set direction to wind up pressure    
}

///////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
uint8_t LimitPWM(uint64_t voltage, uint8_t maxVoltage = 255, uint8_t minVoltage = 0){
    if (voltage > maxVoltage) {
        voltage = maxVoltage; // Limit voltage to maximum value
    } 
    else if (voltage < minVoltage) {
        voltage = minVoltage; // Limit voltage to minimum value
    }
    
    voltage = (uint8_t)voltage; // Cast voltage to uint8_t for PWM output

    return voltage;
}

uint64_t ReadEncoder(uint64_t Encodervalue, gpio_num_t EncoderPinA, gpio_num_t EncoderPinB){
    //Example logic to read encoder value based on the state of the encoder pins
    if(digitalRead(EncoderPinA) == HIGH)
        if (digitalRead(EncoderPinA)> digitalRead(EncoderPinB))
            Encodervalue++;
        else
            Encodervalue--;
            // Example function to read encoder value for feedback
        // Replace with actual implementation to read from the encoder hardware
    return Encodervalue; // Return the read encoder value
}