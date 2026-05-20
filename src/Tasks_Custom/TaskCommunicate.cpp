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

#define COMM_UART_LINE_MAX      96
#define COMM_SEND_INTERVAL_MS   500

// Shared command data exchanged with motion and pressure tasks.
// The receive task updates this struct from incoming UART commands.
CommunicationData_t Machine_Settings = {0, 0.0f};

// Send a NUL-terminated ASCII string over the SC16IS740 UART bridge.
static void uart_SendString(const char *message)
{
    while (*message != '\0')
    {
        uart_SendByte((uint8_t)*message);
        message++;
    }
}

// Convert a 64-bit unsigned integer into a decimal ascii string.
// This is used for sending the requested angle value in STATUS reports.
static void uint64ToString(uint64_t value, char *buffer, size_t bufferSize)
{
    if (bufferSize == 0)
    {
        return;
    }

    buffer[bufferSize - 1] = '\0';
    size_t index = bufferSize - 2;
    buffer[index] = '\0';

    if (value == 0)
    {
        buffer[index--] = '0';
    }
    else
    {
        while (value > 0 && index < bufferSize - 1)
        {
            buffer[index--] = '0' + (uint8_t)(value % 10ULL);
            value /= 10ULL;
        }
    }

    memmove(buffer, &buffer[index + 1], bufferSize - index - 1);
}

// Parse a positive integer value from an ASCII string.
// Returns false if the string contains invalid characters.
static bool parseUint64(const char *str, uint64_t *value)
{
    if (str == NULL || *str == '\0')
    {
        return false;
    }

    uint64_t parsed = 0ULL;
    const char *current = str;

    while (*current >= '0' && *current <= '9')
    {
        parsed = parsed * 10ULL + (uint64_t)(*current - '0');
        current++;
    }

    if (*current != '\0')
    {
        return false;
    }

    *value = parsed;
    return true;
}

// Parse a decimal floating-point value from ASCII.
// Accepts values like "12.34" and returns false on invalid syntax.
static bool parseFloatValue(const char *str, float *value)
{
    if (str == NULL || *str == '\0')
    {
        return false;
    }

    uint64_t integerPart = 0ULL;
    const char *current = str;

    while (*current >= '0' && *current <= '9')
    {
        integerPart = integerPart * 10ULL + (uint64_t)(*current - '0');
        current++;
    }

    float result = (float)integerPart;

    if (*current == '.')
    {
        current++;
        uint64_t fraction = 0ULL;
        uint32_t digits = 0U;

        while (*current >= '0' && *current <= '9')
        {
            if (digits < 8)
            {
                fraction = fraction * 10ULL + (uint64_t)(*current - '0');
                digits++;
            }
            current++;
        }

        float divisor = 1.0f;
        for (uint32_t i = 0U; i < digits; i++)
        {
            divisor *= 10.0f;
        }

        if (digits > 0)
        {
            result += (float)fraction / divisor;
        }
    }

    if (*current != '\0')
    {
        return false;
    }

    *value = result;
    return true;
}

// Format the pressure value as an ASCII string with 2 decimal places.
// This ensures consistent STATUS messages without floating point printing overhead.
static void formatPressureString(char *buffer, size_t bufferSize, float pressure)
{
    if (bufferSize == 0)
    {
        return;
    }

    if (pressure < 0.0f)
    {
        pressure = 0.0f;
    }

    uint32_t value = (uint32_t)(pressure * 100.0f + 0.5f);
    uint32_t integerPart = value / 100U;
    uint32_t fractionPart = value % 100U;

    char integerText[16] = {0};
    size_t index = sizeof(integerText) - 2;
    integerText[index] = '\0';

    if (integerPart == 0U)
    {
        integerText[index--] = '0';
    }
    else
    {
        while (integerPart > 0U && index < sizeof(integerText) - 1)
        {
            integerText[index--] = '0' + (uint8_t)(integerPart % 10U);
            integerPart /= 10U;
        }
    }

    memmove(integerText, &integerText[index + 1], sizeof(integerText) - index - 1);
    snprintf(buffer, bufferSize, "%s.%02u", integerText, fractionPart);
}

// Send a telemetry line over UART containing current machine setpoints.
// Example output: STATUS,ANGLE=123,PRESSURE=45.67
static void sendStatusMessage(void)
{
    char angleText[24];
    char pressureText[16];
    char message[COMM_UART_LINE_MAX];

    uint64ToString(Machine_Settings.Idealangle, angleText, sizeof(angleText));
    formatPressureString(pressureText, sizeof(pressureText), Machine_Settings.IdealPressure);

    int len = snprintf(message, sizeof(message), "STATUS,ANGLE=%s,PRESSURE=%s\n", angleText, pressureText);
    if (len > 0)
    {
        uart_SendString(message);
    }
}

// Simple prefix matcher used for command parsing.
static bool stringStartsWith(const char *text, const char *prefix)
{
    return (strncmp(text, prefix, strlen(prefix)) == 0);
}

// Process SET commands by splitting the payload on commas and updating
// Angle/Pressure setpoints from the received values.
static void processSetCommand(char *payload)
{
    bool updated = false;
    char *token = strtok(payload, ",");

    while (token != NULL)
    {
        if (stringStartsWith(token, "ANGLE="))
        {
            uint64_t angleValue = 0ULL;
            if (parseUint64(token + 6, &angleValue))
            {
                Machine_Settings.Idealangle = angleValue;
                updated = true;
            }
        }
        else if (stringStartsWith(token, "PRESSURE="))
        {
            float pressureValue = 0.0f;
            if (parseFloatValue(token + 9, &pressureValue))
            {
                Machine_Settings.IdealPressure = pressureValue;
                updated = true;
            }
        }

        token = strtok(NULL, ",");
    }

    if (updated)
    {
        uart_SendString("ACK\n");
    }
    else
    {
        uart_SendString("ERROR,INVALID_SET_COMMAND\n");
    }
}

// Interpret a complete command line received over UART.
// Supported commands: SET, GET, PING.
static void executeCommand(char *line)
{
    if (line == NULL || *line == '\0')
    {
        return;
    }

    char *command = line;
    while (*command == ' ' || *command == '\t')
    {
        command++;
    }

    if (stringStartsWith(command, "SET"))
    {
        char *payload = command + 3;
        while (*payload == ' ' || *payload == ':' || *payload == ',')
        {
            payload++;
        }

        if (*payload == '\0')
        {
            uart_SendString("ERROR,NO_PAYLOAD\n");
        }
        else
        {
            processSetCommand(payload);
        }
    }
    else if (stringStartsWith(command, "GET"))
    {
        sendStatusMessage();
    }
    else if (stringStartsWith(command, "PING"))
    {
        uart_SendString("PONG\n");
    }
    else
    {
        uart_SendString("ERROR,UNKNOWN_COMMAND\n");
    }
}

// Periodically transmit the current machine status over the CAN-to-UART bridge.
void TaskCommunicate_Send(void *pvParameters)
{
    (void)pvParameters;

    while (true)
    {
        sendStatusMessage();
        taskSleep(COMM_SEND_INTERVAL_MS);
    }
}

// Continuously read bytes from the UART bridge, build command lines,
// and execute commands once a newline terminator is received.
void TaskCommunicate_Receive(void *pvParameters)
{
    (void)pvParameters;

    char buffer[COMM_UART_LINE_MAX];
    size_t position = 0;

    while (true)
    {
        int16_t received = uart_ReadByte();

        if (received < 0)
        {
            taskSleep(10);
            continue;
        }

        char receivedChar = (char)received;

        if (receivedChar == '\r')
        {
            continue;
        }

        if (receivedChar == '\n')
        {
            buffer[position] = '\0';
            executeCommand(buffer);
            position = 0;
            continue;
        }

        if (position < (COMM_UART_LINE_MAX - 1))
        {
            buffer[position++] = receivedChar;
        }
        else
        {
            position = 0;
        }
    }
}
