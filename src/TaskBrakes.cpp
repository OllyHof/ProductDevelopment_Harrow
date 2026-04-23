///////////////////////////////////////////////////////////////////////////////
//
// TaskBrakes.cpp
//
// Authors: 	Oliver Hofman
// Edit date: 	22-04-2026
//
///////////////////////////////////////////////////////////////////////////////

///////////////////////////////////////////////////////////////////////////////
// system includes

#include <Arduino.h>
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include <stdbool.h>

///////////////////////////////////////////////////////////////////////////////
// application includes

#include "TaskBrakes.h"

///////////////////////////////////////////////////////////////////////////////
// bool taskBrakes (bool BrakeOn, uint8_t BrakePin)

bool taskBrakes (bool BrakeOn, uint8_t BrakePin)
{
	if(BrakeOn){
		digitalWrite(BrakePin, false);
		if(digitalRead(BrakePin) == LOW){
			return false;
		}
	}
	else{
		digitalWrite(BrakePin, true);
		if(digitalRead(BrakePin) == HIGH){
			return false;
		}
	}
	return true;
}

//////////////////////////////////////////////////////////////////////////////
// Implemente TaskBrakes as follows:

// - If BrakeOn is true, set the brake pin to engage the brakes (e.g., LOW)
// - If BrakeOn is false, set the brake pin to disengage the brakes (e
//        if (taskBrakes(//BrakeOn, //BrakePin)){
//            // Give Interrupt Error, brake error
//        }
// - Note: if no error occurs this if statement gets skipped.