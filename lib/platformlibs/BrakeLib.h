///////////////////////////////////////////////////////////////////////////////
//
// TaskBrakes.h
//
// Authors: 	Oliver Hofman
// Edit date: 	22-04-2026
//
///////////////////////////////////////////////////////////////////////////////


#ifndef TASKBRAKES_H_
#define TASKBRAKES_H_

#include "Function_Config.h"

bool taskBrakes (BrakeState_t state, gpio_num_t BrakePin);
bool Estop_Brake ();
bool Reset_Brake (); 
#endif	// TASKBRAKES_H_