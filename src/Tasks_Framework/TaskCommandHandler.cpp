///////////////////////////////////////////////////////////////////////////////
//
// TaskCLIHandler.cpp
//
// Authors: 	Roel Smeets
// Edit date: 	02-06-2025
//
// Joel Brigida
// CDA 4102: Computer Architecture
// Implementation file for Task2: determine valid input.
// June 28, 2023
//
///////////////////////////////////////////////////////////////////////////////


///////////////////////////////////////////////////////////////////////////////
// system includes

#include <Arduino.h>

///////////////////////////////////////////////////////////////////////////////
// application includes

#include "SerialPrintf.h"
#include "TaskSleep.h"
#include "CommandParser.h"
#include "InfoRTOS.h"
#include "TaskCLIHandler.h"
#include "TaskCommandHandler.h"
#include "Function_Config.h"

///////////////////////////////////////////////////////////////////////////////
// void task_CommandHandler(void* param)

void task_CommandHandler(void* param)
{
    char buffer[BUF_LEN];   	// string buffer for terminal message

    while (true)
    {
		memset(buffer, 0, BUF_LEN);	// Clear input buffer

        if(cliMessageQueue != NULL)
        {
            if(xQueueReceive(cliMessageQueue, buffer, 0) == pdTRUE)
            {
				// SerialPrintf(">>> read from queue: [%s]\n", buffer);

				if (cmd_ParseCommand(buffer, "stats"))
				{
					// SerialPrintf("stats command\n");
					info_Tasks();
				}
				else if (cmd_ParseCommand(buffer, "cpu"))
				{
					// SerialPrintf("cpu command\n");
					// int32_t cmd_ParseInteger(String commandLine, String paramName, 
					// 	 int32_t previousValue, int32_t lowerLimit, int32_t upperlimit)
					info_CPU();
				}
				else if (cmd_ParseCommand(buffer, "ver"))
				{
					info_Version();
				}
				else if (cmd_ParseCommand(buffer, "Setup"))
				{
					Machine_Settings.IdealPressure = cmd_ParseFloat(buffer, "pressure", Machine_Settings.IdealPressure, 1.0, 3.5);
					Machine_Settings.Idealangle = cmd_ParseInteger(buffer, "angle", Machine_Settings.Idealangle, 10, 35);
					SerialPrintf("> updated settings: Pressure=%.2f, Angle=%d\n", Machine_Settings.IdealPressure, Machine_Settings.Idealangle);
				}
				else if (cmd_ParseCommand(buffer, "Reset"))
				{
					SerialPrintf("> resetting system...\n");
					esp_restart();
				}
				else if (cmd_ParseCommand(buffer, "Start"))
				{
					SerialPrintf("> starting control loop...\n");
					xSemaphoreGive(xHandleStartControlLoop); // Signal control loop to start
				}
				else if (cmd_ParseCommand(buffer, "help"))
				{
					SerialPrintf("> available commands:\n");
					SerialPrintf("  stats - show RTOS task info\n");
					SerialPrintf("  cpu   - show CPU and system info\n");
					SerialPrintf("  ver   - show software version info\n");
					SerialPrintf("  help  - show this help message\n");
					SerialPrintf("  reset - reset the system\n");
					SerialPrintf("  setup - set pressure and angle\n");
					SerialPrintf("    usage: setup pressure=<value> angle=<value>\n");
					SerialPrintf("  start - start the control loop\n");
				}
				else
                {
					// SerialPrintf("> invalid command\n");
                }
            }
        }
		taskSleep(0);
    } 
}
