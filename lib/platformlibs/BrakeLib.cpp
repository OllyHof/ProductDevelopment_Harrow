///////////////////////////////////////////////////////////////////////////////
//
// BrakeLib.cpp
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

#include "BrakeLib.h"
#include "Hardware_Config.h"
#include "SerialPrintf.h"
#include "Function_Config.h"

BrakeConfig_t BrakeConfigs[] = 
{
    {PIN_BRAKE_UPPER_1, "Brake Upper 1", UNKNOWN, BRAKE_ENGAGED}, // Pressure channel 1
	/* Section commented out since it won't be used in the current machine configuration
	{PIN_BRAKE_UPPER_2, "Brake Upper 2", UNKNOWN, BRAKE_ENGAGED}, // Pressure channel 2
    {PIN_BRAKE_UPPER_3, "Brake Upper 3", UNKNOWN, BRAKE_ENGAGED}, // Pressure channel 3
    {PIN_BRAKE_UPPER_4, "Brake Upper 4", UNKNOWN, BRAKE_ENGAGED}, // Pressure channel 4
	{PIN_BRAKE_LOWER, "Brake Lower", UNKNOWN, BRAKE_ENGAGED} // Angle channel
	*/
};


////////////////////////////////////////////////////////////////////////////////
void Brake_Init()
{
	for(int i = 0; i < sizeof(BrakeConfigs) / sizeof(BrakeConfig_t); i++) // For all brakes
	{
		BrakeConfig_t *config = &BrakeConfigs[i]; // Pointer to correct brake
		pinMode(config->BrakeID, OUTPUT); // Set brake pin as output
		Brake_Set(config->BrakeID, config->BrakeDefault); // Set brake to initial state
	}
}

///////////////////////////////////////////////////////////////////////////////
// bool Brake_Set (gpio_num_t BrakePin, BrakeState_t state)

bool Brake_Set (gpio_num_t BrakePin, BrakeState_t state)
{
	digitalWrite(BrakePin, state); // Set output pin to desired value  

	if (digitalRead(BrakePin) != state) {return true;} // Return true to indicate an error occurred (false indicates success)

	if (!estopActive) // Disable config change for ESTOP
	{
	for(int i = 0; i < sizeof(BrakeConfigs) / sizeof(BrakeConfig_t); i++) // Check if changed brake is stored in config
	{
		BrakeConfig_t *config = &BrakeConfigs[i]; // Pointer to correct brake
		if (BrakePin == config->BrakeID){config->BrakeState=state; break;} // Update config 	
	}
	}

	return false; // Return false to indicate no error occurred (true would indicate a failure to set the brake state)
}

////////////////////////////////////////////////////////////////////////////////
// bool Brake_Estop()
bool Brake_Estop()
{
	bool result = true;
	for(int i = 0; i < sizeof(BrakeConfigs) / sizeof(BrakeConfig_t); i++) // For all brakes
	{
		BrakeConfig_t *config = &BrakeConfigs[i]; // Pointer to correct brake

		if (Brake_Set(config->BrakeID, BRAKE_ENGAGED)) 
		{SerialPrintf("> ERROR: Failed to engage %s!\n", config->BrakeName.c_str()); result = false;}
		else 
		{SerialPrintf("> %s engaged successfully.\n", config->BrakeName.c_str());}
	}
	return result;
}

/////////////////////////////////////////////////////////////////////////////////
// Bool Brake_Reset() 
bool Brake_Reset()
{
	bool result = true;
	for(int i = 0; i < sizeof(BrakeConfigs) / sizeof(BrakeConfig_t); i++) // For all brakes
	{
		BrakeConfig_t *config = &BrakeConfigs[i]; // Pointer to correct brake
		if (Brake_Set(config->BrakeID, config->BrakeState)) // Set brake to original state
		{SerialPrintf("> ERROR %s Failed to return to original state!\n", config->BrakeName.c_str()); result = false;}
		else
		{SerialPrintf("> %s returned to original state: %s\n", config->BrakeName.c_str(), ((config->BrakeState == BRAKE_ENGAGED) ? "Engaged" : "Released"));}
	}
	return result;
}

/////////////////////////////////////////////////////////////////////////////////////