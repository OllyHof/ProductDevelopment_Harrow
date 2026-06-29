///////////////////////////////////////////////////////////////////////////////
//
// TaskCommunicate.cpp
//
// Authors: 	Oliver Hofman
// Edit date: 	20-05-2026
//
///////////////////////////////////////////////////////////////////////////////

///////////////////////////////////////////////////////////////////////////////
// system includes

#include <Arduino.h>
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include <stdbool.h>
#include <string.h>
#include <stdlib.h>
#include <stdio.h>
#include <IOLib.h>

///////////////////////////////////////////////////////////////////////////////
// application includes

#include "TaskCommunicate.h"
#include "UART740Lib.h"
#include "SerialPrintf.h"
#include "TaskSleep.h"
#include "Function_Config.h"

// Empty task for future communication handling, currently not implemented