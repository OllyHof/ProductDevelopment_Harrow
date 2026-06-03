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
				else if (cmd_ParseCommand(buffer, "reset.hard"))
				{
					SerialPrintf("> resetting system...\n");
					esp_restart();
				}
				else if (cmd_ParseCommand(buffer, "reset.soft"))
				{
					SerialPrintf("> resetting ESTOP...\n");
					xSemaphoreGive(xResetSemaphore); // Signal ESTOP handler to reset the system
					taskSleep(10);
					xSemaphoreTake(xResetSemaphore, 0);
				}
				else if (cmd_ParseCommand(buffer, "start"))
				{
					SerialPrintf("> starting control loop...\n");
					xSemaphoreGive(xHandleStartControlLoop); // Signal control loop to start
				}
				else if (cmd_ParseCommand(buffer, "shutdown"))
				{
					SerialPrintf("> Setting Machine settings to resting values...\n");
					Machine_Settings = {0,0};
					SerialPrintf("> Applying Machine Settings...");
					xSemaphoreGive(xHandleStartControlLoop); // Signal control loop to start
				}
				else if (cmd_ParseCommand(buffer, "setup"))
				{
					SerialPrintf("> Starting machine setup\n");
					SerialPrintf("> Input format:\n");
					SerialPrintf("  pressure=<value>\n");
					SerialPrintf("  angle=<value>\n");
					SerialPrintf("> Type 'exit' anytime to quit\n\n");

					bool setupRunning = true;

					while (setupRunning)
					{
						// -------- PRESSURE --------
						SerialPrintf("> Enter pressure:\n");

						if (xQueueReceive(cliMessageQueue, buffer, portMAX_DELAY) == pdTRUE)
						{
							if (cmd_ParseCommand(buffer, "exit"))
							{
								break;
							}

							Machine_Settings.IdealPressure =
								cmd_ParseFloat(
									String(buffer),
									"pressure",
									Machine_Settings.IdealPressure,
									1.0,
									2.4);

							SerialPrintf("> Pressure updated: %.2f\n",
										Machine_Settings.IdealPressure);
						}

						// -------- ANGLE --------
						SerialPrintf("> Enter angle:\n");

						if (xQueueReceive(cliMessageQueue, buffer, portMAX_DELAY) == pdTRUE)
						{
							if (cmd_ParseCommand(buffer, "exit"))
							{
								break;
							}

							Machine_Settings.IdealAngle =
								cmd_ParseInteger(
									String(buffer),
									"angle",
									Machine_Settings.IdealAngle,
									10,
									35);

							SerialPrintf("> Angle updated: %d\n",
										Machine_Settings.IdealAngle);
						}

						// -------- EXIT CHECK --------
						SerialPrintf("> Exit setup? (yes/no)\n");

						if (xQueueReceive(cliMessageQueue, buffer, portMAX_DELAY) == pdTRUE)
						{
							if (cmd_ParseCommand(buffer, "yes"))
							{
								setupRunning = false;
							}
						}
					}

					SerialPrintf("> Setup complete\n");
					SerialPrintf("> Final settings: pressure=%.20f angle=%d\n",
								Machine_Settings.IdealPressure,
								Machine_Settings.IdealAngle);
				}
				else if (cmd_ParseCommand(buffer, "help"))
				{
					SerialPrintf("> available commands:\n");
					SerialPrintf("  stats - show RTOS task info\n");
					SerialPrintf("  cpu   - show CPU and system info\n");
					SerialPrintf("  ver   - show software version info\n");
					SerialPrintf("  help  - show this help message\n");
					SerialPrintf("  reset.hard - reset the system\n");
					SerialPrintf("  reset.soft - reset Estop\n");
					SerialPrintf("  setup - set pressure and angle\n");
					SerialPrintf("  start - start the control loop\n");
					SerialPrintf("  shutdown - make machine ready for storage \n");
				}
				else
                {
					SerialPrintf("> invalid command\n");
                }
            }
        }
		taskSleep(0);
    } 
}
