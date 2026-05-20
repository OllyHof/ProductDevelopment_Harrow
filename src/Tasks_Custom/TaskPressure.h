///////////////////////////////////////////////////////////////////////////////
//
// TaskPressure.h
//
// Authors: 	Oliver Hofman
// Edit date: 	19-05-2026
//
///////////////////////////////////////////////////////////////////////////////

#ifndef TASKPRESSURE_H_
#define TASKPRESSURE_H_
void TaskPressure(void *pvParameters);
uint8_t LimitPWM(uint64_t voltage, uint8_t maxVoltage, uint8_t minVoltage);
void ChangeDirection(bool direction);
uint64_t ReadEncoder(uint64_t Encodervalue, gpio_num_t EncoderPinA, gpio_num_t EncoderPinB);
#endif // TASKPRESSURE_H_