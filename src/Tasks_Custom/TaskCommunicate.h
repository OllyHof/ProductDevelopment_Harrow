///////////////////////////////////////////////////////////////////////////////
//
// TaskCommunicate.h
//
// Authors: 	Oliver Hofman
// Edit date: 	19-05-2026
//
///////////////////////////////////////////////////////////////////////////////

#ifndef TASKCOMMUNICATE_H_
#define TASKCOMMUNICATE_H_

#include <stdint.h>
 
typedef struct
{
    uint64_t Idealangle; // 
    float IdealPressure;
} CommunicationData_t;

extern CommunicationData_t Machine_Settings; // Example initialization of communication data


void TaskCommunicate_Send(void *pvParameters);
void TaskCommunicate_Receive(void *pvParameters);

#endif // TASKCOMMUNICATE_H_
