///////////////////////////////////////////////////////////////////////////////
//
// TaskBrakes.cpp
//
// Authors: 	Oliver Hofman
// Edit date: 	22-04-2026
//
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

#include "TaskBrakes.h"
#include "Hardware_Config.h"
#include "SerialPrintf.h"

typedef struct
{
    gpio_num_t BrakeID;     // Motor select GPIO for the pressure channel
	String BrakeName;		// Name of brake for console log
} BrakeConfig_t;

BrakeConfig_t BrakeConfigs[] = 
{
    {PIN_BRAKE_UPPER_1, "Brake Upper 1"}, // Pressure channel 1
    {PIN_BRAKE_UPPER_2, "Brake Upper 2"}, // Pressure channel 2
    {PIN_BRAKE_UPPER_3, "Brake Upper 3"}, // Pressure channel 3
    {PIN_BRAKE_UPPER_4, "Brake Upper 4"}, // Pressure channel 4
	{PIN_BRAKE_LOWER, "Brake Lower"} // Angle channel
};


///////////////////////////////////////////////////////////////////////////////
// bool taskBrakes (bool BrakeOn, uint8_t BrakePin)

bool taskBrakes (bool BrakeOn, uint8_t BrakePin)
{
	// Set the brake control pin according to the requested state.
	// NOTE: physical brake polarity depends on wiring; this function
	// simply writes the requested logical state and reports success.
	io_SetBit(BrakePin, BrakeOn);

	if (digitalRead(BrakePin) != BrakeOn)
		return true; // Return true to indicate an error occurred (false indicates success)
	
	return false; // Return false to indicate no error occurred (true would indicate a failure to set the brake state)
}

//////////////////////////////////////////////////////////////////////////////
// Implement TaskBrakes as follows:

// - If BrakeOn is true, set the brake pin to engage the brakes (e.g., LOW)
// - If BrakeOn is false, set the brake pin to disengage the brakes (e
//        if (taskBrakes(//BrakeOn, //BrakePin)){
//            // Give Interrupt Error, brake error
//        }
// - Note: if no error occurs this if statement gets skipped.


////////////////////////////////////////////////////////////////////////////////
void Estop_Brake()
{
	for(int i = 0; i < sizeof(BrakeConfigs) / sizeof(BrakeConfig_t); i++)
	{
		BrakeConfig_t *config = &BrakeConfigs[i];

		if (taskBrakes(false, config->BrakeID))
		{
			SerialPrintf("> ERROR: Failed to disengage %s!\n", config->BrakeName.c_str());
		}
		else 
		{
			SerialPrintf("> %s  successfully.\n", config->BrakeName.c_str());
		}
	}
}