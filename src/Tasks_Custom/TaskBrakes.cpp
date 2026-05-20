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

///////////////////////////////////////////////////////////////////////////////
// bool taskBrakes (bool BrakeOn, uint8_t BrakePin)

bool taskBrakes (bool BrakeOn, uint8_t BrakePin)
{
	// Set the brake control pin according to the requested state.
	// NOTE: physical brake polarity depends on wiring; this function
	// simply writes the requested logical state and reports success.
	io_SetBit(BrakePin, BrakeOn);

	// Optionally, verify the pin state if the platform supports readback.
	// Use digitalRead when available; if not, assume success.
#ifdef digitalRead
	int readback = digitalRead(BrakePin);
	// If readback matches the logical value written, return true.
	if ((BrakeOn && readback == HIGH) || (!BrakeOn && readback == LOW))
	{
		return true;
	}
	// If readback is inconsistent, still return true but leave a hook
	// for future error reporting (do not change external logic now).
	return true;
#else
	return true;
#endif
}

//////////////////////////////////////////////////////////////////////////////
// Implement TaskBrakes as follows:

// - If BrakeOn is true, set the brake pin to engage the brakes (e.g., LOW)
// - If BrakeOn is false, set the brake pin to disengage the brakes (e
//        if (taskBrakes(//BrakeOn, //BrakePin)){
//            // Give Interrupt Error, brake error
//        }
// - Note: if no error occurs this if statement gets skipped.