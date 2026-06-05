///////////////////////////////////////////////////////////////////////////////
//
// MotorUtils.cpp
//
// Shared motor utility implementations for PWM limiting, direction control,
// and interrupt-driven encoder reading.
//
///////////////////////////////////////////////////////////////////////////////

#include <Arduino.h>
#include <stdint.h>
#include "driver/gpio.h"
#include "freertos/FreeRTOS.h"
#include <IOLib.h>
#include "MotorUtils.h"

static volatile int64_t s_encoderCount = 0;
static gpio_num_t s_encoderPinA = (gpio_num_t)-1;
static gpio_num_t s_encoderPinB = (gpio_num_t)-1;
static portMUX_TYPE s_encoderMux = portMUX_INITIALIZER_UNLOCKED;

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

void InitEncoder(gpio_num_t EncoderPinA, gpio_num_t EncoderPinB)
{
    s_encoderPinA = EncoderPinA;
    s_encoderPinB = EncoderPinB;

    pinMode(EncoderPinA, INPUT_PULLUP);
    pinMode(EncoderPinB, INPUT_PULLUP);

    ResetEncoder(0);

    int irq = digitalPinToInterrupt(EncoderPinA);
    if (irq != NOT_AN_INTERRUPT)
    {
        attachInterrupt(irq, EncoderISR, CHANGE);
    }
}

void DeinitEncoder()
{
    if (s_encoderPinA != (gpio_num_t)-1)
    {
        int irq = digitalPinToInterrupt(s_encoderPinA);
        if (irq != NOT_AN_INTERRUPT)
        {
            detachInterrupt(irq);
        }
        pinMode(s_encoderPinA, INPUT);
    }

    if (s_encoderPinB != (gpio_num_t)-1)
    {
        pinMode(s_encoderPinB, INPUT);
    }

    s_encoderPinA = (gpio_num_t)-1;
    s_encoderPinB = (gpio_num_t)-1;
    ResetEncoder(0);
}

void ResetEncoder(int64_t initialValue)
{
    portENTER_CRITICAL(&s_encoderMux);
    s_encoderCount = initialValue;
    portEXIT_CRITICAL(&s_encoderMux);
}

int64_t GetEncoderValue()
{
    int64_t value;

    portENTER_CRITICAL(&s_encoderMux);
    value = s_encoderCount;
    portEXIT_CRITICAL(&s_encoderMux);

    return value;
}

int64_t ReadEncoder()
{
    return GetEncoderValue();
}

void IRAM_ATTR EncoderISR()
{
    if (s_encoderPinA == (gpio_num_t)-1 || s_encoderPinB == (gpio_num_t)-1)
    {
        return;
    }

    int aLevel = gpio_get_level(s_encoderPinA);
    int bLevel = gpio_get_level(s_encoderPinB);
    int64_t delta = (aLevel == bLevel) ? -1 : 1;

    portENTER_CRITICAL_ISR(&s_encoderMux);
    s_encoderCount += delta;
    portEXIT_CRITICAL_ISR(&s_encoderMux);
}