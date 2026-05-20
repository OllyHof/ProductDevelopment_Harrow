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

extern TaskHandle_t handle_PressureTask;
extern TaskHandle_t handle_AngleTask;
extern TaskHandle_t handle_ControlLoopTask;

///////////////////////////////////////////////////////////////////////////////
// LED Configuration structure
typedef struct {
    bool red;
    bool green;
    bool yellow;
    bool blink;
} LEDConfig;

// Lookup table indexed by status code
// Columns: RED, GREEN, YELLOW, BLINK
// Edit the boolean values below to change LED patterns for each status
const LEDConfig ledConfigs[] = {
    {false, false, true,  true},   // STATUS_NOCONTROL (0x00) - Yellow Blinking
    {true,  false, false, true},   // STATUS_ERROR_HARD (0x01) - Red Blinking
    {true,  false, false, false},  // STATUS_ERROR_SOFT (0x02) - Red Solid
    {false, true,  false, false},  // STATUS_ALLGOOD (0x03) - Green Solid
    {false, false, true,  false},  // STATUS_STANDBY (0x04) - Yellow Solid
    {false, true,  false, true},   // STATUS_RUNNING (0x05) - Green Blinking
    {true,  false, true,  false},  // STATUS_WARNING (0x06) - Red + Yellow Solid
    {false, false, false, false},  // STATUS_DISABLED (0x07) - All LEDs Off
    {true,  true,  true,  true},   // STATUS_MAINTENANCE (0x08) - All Colors Blinking
    {false, true,  true,  true},   // STATUS_WORKING (0x09) - Green + Yellow Blinking
};

#define NUM_LED_CONFIGS (sizeof(ledConfigs) / sizeof(ledConfigs[0]))

static bool isTaskActive(TaskHandle_t handle)
{
    if (handle == NULL)
    {
        return false;
    }

    eTaskState state = eTaskGetState(handle);
    return (state != eDeleted && state != eInvalid);
}

static bool hasFatalError(void)
{
    // Replace with actual fatal error detection using sensors, watchdogs, or error flags.
    return false;
}

static bool hasSoftError(void)
{
    // Replace with actual non-critical error detection if required.
    return false;
}

static uint8_t determineMachineStatus(void)
{
    if (hasFatalError())
    {
        return STATUS_ERROR_HARD;
    }

    if (hasSoftError())
    {
        return STATUS_ERROR_SOFT;
    }

    if (isTaskActive(handle_PressureTask) || isTaskActive(handle_AngleTask))
    {
        return STATUS_RUNNING;
    }
    
    return STATUS_ALLGOOD;
}

////////////////////////////////////////////////////////////////////////////////
void MachineStatus(void *pvParameters)
{
    uint8_t currentStatus = STATUS_NOCONTROL; // Default status

    while (true)
    {
        uint8_t newStatus = determineMachineStatus();

        if (newStatus != currentStatus)
        {
            currentStatus = newStatus;
        }

        taskStatusLight(currentStatus);
        vTaskDelay(pdMS_TO_TICKS(200));
    }
}

///////////////////////////////////////////////////////////////////////////////
// void taskStatusLight(uint8_t Status)
void taskStatusLight(uint8_t Status)
{
    // Default to STATUS_NOCONTROL if out of range
    if(Status >= NUM_LED_CONFIGS) {
        Status = STATUS_NOCONTROL;
    }
    
    LEDConfig config = ledConfigs[Status];
    #if (HARDWARE_CONNECTED == HARDWARE_HARROW)
        io_SetBit(PIN_TREE_RED, config.red);
        io_SetBit(PIN_TREE_GREEN, config.green);
        io_SetBit(PIN_TREE_YELLOW, config.yellow);
        io_SetBit(PIN_TREE_BLINK, config.blink);
    #else
        // If no hardware, print status to serial for testing
        SerialPrintf("Status: 0x%02X - R:%d G:%d Y:%d Blink:%d\n", Status, config.red, config.green, config.yellow, config.blink);
    #endif // HARDWARE_CONNECTED

}