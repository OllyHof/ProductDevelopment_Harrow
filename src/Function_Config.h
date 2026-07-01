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

// Status codes for machine operation
typedef enum {
    STATUS_NOCONTROL = 0x00, // System initializing
    STATUS_ERROR_HARD = 0x01, // Critical error
    STATUS_ALLGOOD = 0x02, // idle/ready
    STATUS_RUNNING = 0x03, // actively operating
    STATUS_MAINTENANCE = 0x04, //diagnostic/service mode
    STATUS_ERROR_SOFT = 0x05, // Non-critical error
} MachineStatus_t;

// Current Machine Status
extern MachineStatus_t PreviousMachineStatus;
extern MachineStatus_t CurrentMachineStatus;

// Status codes for machine operation
typedef struct {
    bool red;
    bool green;
    bool yellow;
    bool blink;
} LEDConfig_t;

///////////////////////////////////////////////////////////////////////////////
// Control Loop Thresholds - scaled for full operating ranges

#define ANGLE_ERROR_THRESHOLD    0     
#define PRESSURE_ERROR_THRESHOLD 0
#define INITIALANGLEOFFSET       0    // degrees
///////////////////////////////////////////////////////////////////////////////
// TaskCommunicate
typedef struct
{
    uint32_t IdealAngle; 
    float IdealPressure;
} CommunicationData_t;
#define MAX_MESSAGE_RATE_MS 100 // Maximum message rate in milliseconds (10 messages per second)


extern CommunicationData_t Machine_Settings;
extern SemaphoreHandle_t xHandleStartControlLoop;
extern SemaphoreHandle_t xEstopSemaphore; // Semaphore to trigger ESTOP
extern SemaphoreHandle_t xResetSemaphore; // Semaphore to signal ESTOP reset
extern SemaphoreHandle_t xDebugSemaphore; // Semaphore to signal assessment task
extern volatile bool estopActive; // Flag to indicate if ESTOP is active
extern volatile bool estopDone; // Flag to incidacate if ESTOP actions have completed
extern bool motorinfoEnabled; // Flag to indicate if motor info display is enabled
extern bool RealTimeModeEnabled; // Flag to indicate if real-time mode is enabled

#define BRAKE_FAIL_ESTOP false // Set to true to trigger an ESTOP if a brake fails to engage/disengage during operation

#define AnalogWriteResolution 8 // Set the PWM resolution to 8 bits (0-255)
#define AnalogWriteFrequency 200000 // Set the PWM frequency to 200 kHz
#define AnalogWriteMaxValue (2^AnalogWriteResolution - 1) // Set the maximum PWM value to 255 for 8-bit resolution
#define AnalogWriteMinValue 0 // Set the minimum PWM value to 0

typedef enum
{
    UNKNOWN = -1,
    BRAKE_RELEASED = 0,
    BRAKE_ENGAGED  = 1
} BrakeState_t;

typedef struct
{
    gpio_num_t MotorID;     // Motor select GPIO for the pressure channel
    gpio_num_t BrakeID;     // Brake control GPIO for the pressure channel
    int64_t EncoderValue;   // Latest encoder count used for closed-loop control
} MotorConfig_t;

typedef struct
{
    gpio_num_t BrakeID;     // Motor select GPIO for the pressure channel
	String BrakeName;		// Name of brake for console log
    BrakeState_t BrakeState;    // Brake State for Estop/Reset logic
    BrakeState_t BrakeDefault;  // Brake State for Initialization
} BrakeConfig_t;




#endif // FUNCTION_CONFIG_H