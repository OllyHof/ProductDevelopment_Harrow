///////////////////////////////////////////////////////////////////////////////
//
// TaskStatusLight.cpp
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

#include "TaskStatusLight.h"
#include "Function_Config.h"
#include "Hardware_Config.h"
#include "SerialPrintf.h"
#include "InfoRTOS.h"

MachineStatus_t PreviousMachineStatus = STATUS_NOCONTROL;
MachineStatus_t CurrentMachineStatus = STATUS_NOCONTROL;

extern TaskHandle_t handle_PressureTask;
extern TaskHandle_t handle_AngleTask;
extern TaskHandle_t handle_ControlLoopTask;
bool ForceStatusUpdate = false; // Flag to force status update even if the status hasn't changed

///////////////////////////////////////////////////////////////////////////////
// Lookup table indexed by status code
// Columns: RED, GREEN, YELLOW, BLINK
// Edit the boolean values below to change LED patterns for each status
const LEDConfig_t ledConfigs[] = {
    {false, false, true,  true},   // STATUS_NOCONTROL (0x00) - Yellow Blinking
    {true,  false, false, true},   // STATUS_ERROR_HARD (0x01) - Red Blinking
    {false, true,  false, false},  // STATUS_ALLGOOD (0x02) - Green Solid
    {true,  true,  false, false},  // STATUS_RUNNING (0x03) - Green Blinking
    {false, false, false, true},   // STATUS_MAINTENANCE (0x04) - All Colors Blinking
    {false, false, false, false},  // STATUS_ERROR_SOFT (0x05) - Red Solid
};

#define NUM_LED_CONFIGS (sizeof(ledConfigs) / sizeof(ledConfigs[0]))

///////////////////////////////////////////////////////////////////////////////
// void taskStatusLight(void *pvParameters)
void StatusLightHandler(void *pvParameters)
{   
    if ((CurrentMachineStatus != PreviousMachineStatus) || ForceStatusUpdate)
    {
        LEDConfig_t config = ledConfigs[CurrentMachineStatus];
        /* Section commented out because IO limits for the current machine configuration do not allow for LED control
        digitalWrite(PIN_TREE_RED, config.red);
        digitalWrite(PIN_TREE_GREEN, config.green);
        digitalWrite(PIN_TREE_YELLOW, config.yellow);
        digitalWrite(PIN_TREE_BLINK, config.blink);
        */
        SerialPrintf("Status: 0x%02X - R:%d G:%d Y:%d Blink:%d\n", CurrentMachineStatus, config.red, config.green, config.yellow, config.blink);
        
        PreviousMachineStatus = CurrentMachineStatus; // Update previous status to current
        ForceStatusUpdate = false; // Reset the force update flag
    }
    delay(100); // Update every 100 ms
}

// void SetMachineStatus(MachineStatus_t status, bool OverrideError)
void SetMachineStatus(MachineStatus_t status, bool OverrideError)
{
    if (OverrideError || (CurrentMachineStatus != STATUS_ERROR_HARD))
    {
        CurrentMachineStatus = status;
        ForceStatusUpdate = OverrideError; // Force status update if overriding error
    }
}