///////////////////////////////////////////////////////////////////////////////
//
// BrakeLib.h
//
// Authors: 	Oliver Hofman
// Edit date: 	22-04-2026
//
///////////////////////////////////////////////////////////////////////////////


#ifndef BRAKELIB_H_
#define BRAKELIB_H_

#include "Function_Config.h"

bool Brake_Set (gpio_num_t BrakePin, BrakeState_t state);
bool Brake_Estop ();
bool Brake_Reset (); 
void Brake_Init();
#endif	// BRAKELIB_H_