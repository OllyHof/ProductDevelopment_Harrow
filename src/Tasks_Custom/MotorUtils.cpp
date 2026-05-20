///////////////////////////////////////////////////////////////////////////////
//
// MotorUtils.cpp
//
// Shared motor utility implementations for PWM limiting, direction control,
// and encoder reading.
//
///////////////////////////////////////////////////////////////////////////////

#include <Arduino.h>
#include <stdint.h>
#include "driver/gpio.h"
#include <IOLib.h>
#include "MotorUtils.h"

uint8_t LimitPWM(uint64_t voltage, uint8_t maxVoltage, uint8_t minVoltage)
{
    if (voltage > maxVoltage)
    {
        voltage = maxVoltage;
    }
    else if (voltage < minVoltage)
    {
        voltage = minVoltage;
    }

    return (uint8_t)voltage;
}

void ChangeDirection(gpio_num_t directionPin, bool direction)
{
    io_SetBit(directionPin, direction);
}

uint64_t ReadEncoder(uint64_t encoderValue, gpio_num_t EncoderPinA, gpio_num_t EncoderPinB)
{
    if (digitalRead(EncoderPinA) == HIGH)
    {
        if (digitalRead(EncoderPinA) > digitalRead(EncoderPinB))
        {
            encoderValue++;
        }
        else
        {
            encoderValue--;
        }
    }

    return encoderValue;
}
