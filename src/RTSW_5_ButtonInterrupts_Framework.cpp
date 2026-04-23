///////////////////////////////////////////////////////////////////////////////
//
// RTSW_5_ButtonInterrupts_Framework.cpp
//
// Authors: 	Roel Smeets
//              Oliver Hofman
//              
// Edit date: 	02-06-2025
//				10-08-2025
//				20-11-2025
//              31-03-2026
//              22-04-2026
//
///////////////////////////////////////////////////////////////////////////////

///////////////////////////////////////////////////////////////////////////////
// system #includes

#include <Arduino.h>
#include "esp_timer.h"
#include "esp_system.h"
#include "esp_log.h"
#include "esp_task_wdt.h"
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "freertos/semphr.h"
#include "driver/uart.h"
#include "WiFi.h"


///////////////////////////////////////////////////////////////////////////////
// application #includes

#include "SerialPrintf.h"
#include "TaskSleep.h"
#include "TaskHeartbeat.h"
#include "Config.h"
#include "TaskCLIHandler.h"
#include "TaskCommandHandler.h"
#include "InfoRTOS.h"
#include "IOLib.h"
#include "InterruptLib.h"
#include "LEDLib.h"
#include "ButtonLib.h"
#include "SPILib.h"
#include "I2CLib.h"
#include "OLEDLibESP32.h"
#include "I2CScanner.h"
#include "QC7366Lib.h"
#include "UART740Lib.h"
#include "SystemTests.h"
#include "DAC4922Lib.h"
#include "SPIeeprom.h"
#include "ADC3208Lib.h"
#include "InterruptLib.h"

// Custom commands
#include "TaskBrakes.h"

///////////////////////////////////////////////////////////////////////////////
// Global declarations, task handles

xTaskHandle handle_HartbeatTask	= NULL;
xTaskHandle handle_CLITask		= NULL;
xTaskHandle handle_CmdTask		= NULL;

void StartUserTasks(void);

///////////////////////////////////////////////////////////////////////////////
// wrapper, simplified version of xTaskCreatePinnedToCore

bool platformTaskCreate(TaskFunction_t taskCode, void *taskParameters,
                        const char * const taskName, 
                        TaskHandle_t * const taskHandle)
{
    BaseType_t taskResult = pdFAIL;
    bool taskOK = false;
  
    taskResult = xTaskCreatePinnedToCore(taskCode, taskName, 
                RTOS_DEFAULT_STACKSIZE, taskParameters, 1, taskHandle, CORE_1);
    
    info_RegisterTaskByName(taskName);

    taskOK = (taskResult == pdPASS);
    SerialPrintf("> task [%s] creation %s\n", taskName, taskOK ? "OK" : "FAILED");

    return taskOK;
}

///////////////////////////////////////////////////////////////////////////////
// bool platformInit(void)

bool platformInit(void)
{
    bool i2cOK  = false;
    bool oledOK = false;
    bool uartOK = false;
    bool result = true;
    uint8_t nDevices = 0;

    io_Init();
    led_Init();
    button_Init();
    spi_Init();
    qc_Init();
	dac_Init();

    i2cOK  = i2c_Init();
    oledOK = oled_Init();
    uartOK = uart_Init();

	result = i2cOK && oledOK && uartOK;

    nDevices = i2c_ScanBus();

    SerialPrintf("> I2C  init %s\n", i2cOK  ? "OK" : "*** FAILED ***");
    SerialPrintf("> OLED init %s\n", oledOK ? "OK" : "*** FAILED ***");
    SerialPrintf("> UART init %s\n", uartOK ? "OK" : "*** FAILED ***");

    SerialPrintf("> I2C devices found: %d\n", nDevices);

    #if (SYSTEMTEST_ONLY == 1)
    SerialPrintf("> DAC resolution = %10.6f Volt\n", DAC_RESOLUTION);
    SerialPrintf("> DAC min        = %10.6f Volt\n", DAC_MIN_VOLTAGE);
    SerialPrintf("> DAC max        = %10.6f Volt\n", DAC_MAX_VOLTAGE);
    #endif

    cliMessageQueue = xQueueCreate(QUEUESIZE, sizeof(CLI_MESSAGE));

    result &= platformTaskCreate(task_Heartbeat,      NULL, "task_heartbeat", &handle_HartbeatTask);
    result &= platformTaskCreate(task_CLIHandler,     NULL, "task_cli",       &handle_CLITask);
    result &= platformTaskCreate(task_CommandHandler, NULL, "task_cmd",       &handle_CmdTask);

    // manually register default system tasks:
    info_RegisterTaskByName("main");
    info_RegisterTaskByName("esp_timer");
    info_RegisterTaskByName("IDLE0");
    info_RegisterTaskByName("IDLE1");
    info_RegisterTaskByName("Tmr Svc");
    info_RegisterTaskByName("ipc0");
    info_RegisterTaskByName("ipc1");
    info_RegisterTaskByName("loopTask");

    return result;
}

///////////////////////////////////////////////////////////////////////////////
// void setup()

void setup()
{
    bool result = false;

	Serial.begin(115200);
	SerialPrintf("> RTSW Wiedeg\n");
	SerialPrintf("> build: %s\n", __TIMESTAMP__);
	SerialPrintf("> running setup\n");

    result = platformInit();

   	SerialPrintf("> setup done: %s\n", (result == true) ? "OK" : "FAILED");

    // start user tasks here:
    StartUserTasks();

	// info_Tasks();

    #if (SYSTEMTEST_ONLY == 1)
	SerialPrintf("> running system tests...\n");
	// RunSystemTests();	// for separate HW testing
	RunSystemTestsMenu();	// via user menu
	#endif
}
///////////////////////////////////////////////////////////////////////////////
// pin definitions
#define PIN_LED 1
#define PIN_ANGLE_SENSOR 2
#define PIN_PRESSURE_SENSOR 3
#define PIN_ANGLE_BWD 4
#define PIN_ANGLE_FWD 5
#define PIN_PRESSURE 6
#define PIN_ESTOP 7
#define PIN_BrakeAngle 8
#define PIN_BrakePressure 9

///////////////////////////////////////////////////////////////////////////////
// variable declarations
uint16_t rawAngle       = 0;  // Raw Encoder Value
uint16_t rawPressure    = 0;  // Raw Encoder Value
uint16_t angle          = 0;  // True Angle
uint16_t pressure       = 0;  // True Pressure
uint16_t movingAngle    = 0;  // Motor Voltage After PD/PID
uint16_t movingPressure = 0;  // Motor Voltage for Pressure
uint16_t idealPressure  = 0;
uint16_t idealAngle     = 0;
uint16_t pwmFrequency   = 500;
uint16_t pwmPeriod      = 1000/pwmFrequency;
uint16_t pwmResolution  = 8;
//uint8_t Sema_MaxCountOne = 1;
//uint8_t Sema_InitFreeZero = 0;
///////////////////////////////////////////////////////////////////////////////
// user handle declarations

// Task handles
xTaskHandle handle_TaskCANRead          = NULL;
xTaskHandle handle_TaskCANSend          = NULL;
xTaskHandle handle_TaskEStop            = NULL;
xTaskHandle handle_TaskChangePressure   = NULL;
xTaskHandle handle_TaskChangeAngle      = NULL;

// Semaphore Handles
SemaphoreHandle_t Sema_ChangeMachineSettings = NULL;
SemaphoreHandle_t Sema_LowerPressureSprings  = NULL;
SemaphoreHandle_t Sema_EStop                = NULL;

///////////////////////////////////////////////////////////////////////////////
// function prototypes

void TaskCANRead(void *pvParameters);
void TaskCANSend(void *pvParameters);
void TaskEStop(void *pvParameters);
void TaskChangePressure(void *pvParameters);
void TaskChangeAngle(void *pvParameters);

void IRAM_ATTR estopISR(void);

///////////////////////////////////////////////////////////////////////////////
// void StartUserTasks(void)

void StartUserTasks(void)
{
    BaseType_t result = pdFAIL;
    analogWriteFrequency(pwmFrequency); // Set Frequency to 500Hz
    analogWriteResolution(pwmResolution); // Set Resolution to 8 Bits

    Sema_ChangeMachineSettings = xSemaphoreCreateBinary();
    Sema_LowerPressureSprings = xSemaphoreCreateBinary();
    Sema_EStop               = xSemaphoreCreateBinary();

    pinMode(PIN_ESTOP, INPUT_PULLUP);
    interrupt_AttachHandler(estopISR, PIN_ESTOP, FALLING);

    result = platformTaskCreate(TaskCANRead,         NULL, "TaskCANRead",         &handle_TaskCANRead);
    result = platformTaskCreate(TaskCANSend,         NULL, "TaskCANSend",         &handle_TaskCANSend);
    result = platformTaskCreate(TaskEStop,           NULL, "TaskEStop",           &handle_TaskEStop);
    result = platformTaskCreate(TaskChangePressure,  NULL, "TaskChangePressure",  &handle_TaskChangePressure);
    result = platformTaskCreate(TaskChangeAngle,     NULL, "TaskChangeAngle",     &handle_TaskChangeAngle);
 }

///////////////////////////////////////////////////////////////////////////////
void IRAM_ATTR estopISR(void)
{
    BaseType_t xHigherPriorityTaskWoken = pdFALSE;

    if (Sema_EStop != NULL)
    {
        xSemaphoreGiveFromISR(Sema_EStop, &xHigherPriorityTaskWoken);
        portYIELD_FROM_ISR(xHigherPriorityTaskWoken);
    }
}

///////////////////////////////////////////////////////////////////////////////
void TaskCANRead(void *pvParameters)
{
    while (true)
    {
       // Read CAN messages and update variables
        if (/* message received */ true) {
            xSemaphoreGive(Sema_ChangeMachineSettings); // Signal angle/pressure change
        }
        taskSleep(100); // Adjust delay as needed
    }
}

void TaskCANSend(void *pvParameters)
{
    while (true)
    {
        if (/* done */ true ){

            // Create and send CAN message based on current state
        } 
        
        taskSleep(100); // Adjust delay as needed
    }  
}

void TaskEStop(void *pvParameters)
{
    while (true)
    {
        if (xSemaphoreTake(Sema_EStop, portMAX_DELAY) == pdTRUE)
        {
            SerialPrintf("> EStop interrupt received\n");
            taskBrakes(true, PIN_BrakeAngle);
            taskBrakes(true, PIN_BrakePressure);
            // Add additional emergency stop actions here
        }

        taskSleep(100); // Adjust delay as needed
    }
}

void TaskChangeAngle(void *pvParameters)
{
    while (true)
    {
        // Await command to change angle (e.g., from CLI or CAN)
        xSemaphoreTake(Sema_ChangeMachineSettings, portMAX_DELAY);

        // TaskGetInfo(); // Get current angle and pressure
        uint64_t CurrentPressure = 0;

        if (CurrentPressure != 10) {
            xSemaphoreGive(Sema_LowerPressureSprings);
        }

        if (taskBrakes(false, PIN_BrakeAngle)){
            // Give interrupt Error, brake error
        } // Disable brakes
        
        // Move angle to desired position
        
        if (taskBrakes(true, PIN_BrakeAngle)){
            // Give interrupt Error, brake error
        } // Enable brakes
        
        // Give semaphore to pressure task to update pressure based on new angle
        
        taskSleep(100); // Adjust delay as needed
    }
}

void TaskChangePressure(void *pvParameters)
{
    while (true)
    {
        // Await semaphore to change pressure from angle task

        // TaskGetInfo(); // Get current angle and pressure 
        
        if (taskBrakes(false, PIN_BrakePressure)){
            // Give interrupt Error, brake error
        } // Disable brakes

        // Move pressure to desired position

        if (taskBrakes(true, PIN_BrakePressure)){
            // Give interrupt Error, brake error
        } // Enable brakes

        // Give semaphore to pressure task to update pressure based on new angle
        taskSleep(100); // Adjust delay as needed 
    }  
}

///////////////////////////////////////////////////////////////////////////////
// void loop()

void loop()
{
    // totally empty!!!
}
