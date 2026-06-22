///////////////////////////////////////////////////////////////////////////////
//
// MotorUtils.h
//
// Shared motor utility routines for PWM limiting, direction control,
// and encoder reading.
//
///////////////////////////////////////////////////////////////////////////////

#ifndef MOTORUTILS_H_
#define MOTORUTILS_H_

#include <stdint.h>
#include "driver/gpio.h"

uint8_t LimitPWM(uint64_t voltage, uint8_t maxVoltage = 255, uint8_t minVoltage = 0);
void ChangeDirection(gpio_num_t directionPin, uint8_t direction);
void InitEncoder(gpio_num_t EncoderPinA, gpio_num_t EncoderPinB);
void DeinitEncoder();
void ResetEncoder(int64_t initialValue = 0);
int64_t ReadEncoder();
int64_t GetEncoderValue();
void EncoderISR();

#endif // MOTORUTILS_H_
