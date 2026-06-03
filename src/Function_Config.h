///////////////////////////////////////////////////////////////////////////////
//
// Function_Config.h
//
// Authors: 	Oliver Hofman
// Edit date: 	19-05-2026
//
///////////////////////////////////////////////////////////////////////////////


///////////////////////////////////////////////////////////////////////////////
// system #includes

#include <Arduino.h>

///////////////////////////////////////////////////////////////////////////////

#ifndef FUNCTION_CONFIG_H
#define FUNCTION_CONFIG_H

// TaskStatusLight
#define STATUS_NOCONTROL         0x00 // Yellow Blinking (system initializing)
#define STATUS_ERROR_HARD        0x01 // Red Blinking (critical error)
#define STATUS_ERROR_SOFT        0x02 // Red Solid (non-critical error)
#define STATUS_ALLGOOD           0x03 // Green Solid (idle/ready)
#define STATUS_STANDBY           0x04 // Yellow Solid (standby mode)
#define STATUS_RUNNING           0x05 // Green Blinking (actively operating)
#define STATUS_WARNING           0x06 // Red + Yellow Solid (warning condition)
#define STATUS_DISABLED          0x07 // All LEDs Off (offline/disabled)
#define STATUS_MAINTENANCE       0x08 // All Colors Blinking (diagnostic/service mode)

// TaskCommunicate
typedef struct
{
    uint32_t IdealAngle; // 
    float IdealPressure;
} CommunicationData_t;

extern CommunicationData_t Machine_Settings;
extern SemaphoreHandle_t xHandleStartControlLoop;
extern SemaphoreHandle_t xResetSemaphore; // Semaphore to signal ESTOP reset


#endif // FUNCTION_CONFIG_H