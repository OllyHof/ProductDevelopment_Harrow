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
void ChangeDirection(gpio_num_t directionPin, bool direction);
uint64_t ReadEncoder(uint64_t encoderValue, gpio_num_t EncoderPinA, gpio_num_t EncoderPinB);

#endif // MOTORUTILS_H_
