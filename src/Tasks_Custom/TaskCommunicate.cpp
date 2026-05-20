///////////////////////////////////////////////////////////////////////////////
//
// TaskCommunicate.cpp
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

#include "TaskCommunicate.h"

CommunicationData_t Machine_Settings; // Example initialization of communication data
///////////////////////////////////////////////////////////////////////////////
// void TaskCommunicate_Send(void *pvParameters);
void TaskCommunicate_Send(void *pvParameters)
{
    while (true)
    {
        // Send data to the communication interface
        // Example: IOLib_SendData(data);

        vTaskDelay(pdMS_TO_TICKS(10)); // Delay for 10 milliseconds
    }
};
///////////////////////////////////////////////////////////////////////////////
// void TaskCommunicate_Receive(void *pvParameters);
void TaskCommunicate_Receive(void *pvParameters)
{
    while (true)
    {
        Machine_Settings.Idealangle = 0; // Example value
        Machine_Settings.IdealPressure = 0.00; // Example value

        vTaskDelay(pdMS_TO_TICKS(10)); // Delay for 10 milliseconds
    }
}