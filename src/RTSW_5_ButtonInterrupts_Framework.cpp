///////////////////////////////////////////////////////////////////////////////
//
// RTSW_5_ButtonInterrupts_Framework.cpp
//
// Authors: 	Roel Smeets
//              Oliver Hofman
// Edit date: 	02-06-2025
//				10-08-2025
//				20-11-2025
//              22-04-2026 .... 08-05-2026 (Harrow V0.1) Initial Attempt at project during P2.3
//              19-05-2026 .... 20-05-2026 (Harrow V0.2) First fully defined version (non hardware tested)
//				xx-xx-2026 .... xx-xx-2026 (Harrow V0.3) First Bugfix round after testing
//
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
#include "driver/pcnt.h"
//#include "WiFi.h" -- Not included in libs??


///////////////////////////////////////////////////////////////////////////////
// application #includes


#include "SerialPrintf.h"
#include "TaskSleep.h"
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
#include "DAC4922Lib.h"
#include "SPIeeprom.h"
#include "ADC3208Lib.h"

#include "SystemTests.h"

#include "Function_Config.h"
#include "Hardware_Config.h"
#include "Config.h"

#include "Tasks_Framework/TaskHeartbeat.h"
#include "Tasks_Framework/TaskCLIHandler.h"
#include "Tasks_Framework/TaskCommandHandler.h"

#if (HARDWARE_CONNECTED == HARDWARE_HARROW)
    #include "Tasks_Custom/TaskBrakes.h" // implemented; verify hardware mapping
    #include "Tasks_Custom/TaskCommunicate.h" // implemented
    #include "Tasks_Custom/TaskPressure.h" // implemented
    #include "Tasks_Custom/TaskAngle.h" // implemented (needs review)
    #include "Tasks_Custom/TaskStatusLight.h"
#endif // HARDWARE_CONNECTED

///////////////////////////////////////////////////////////////////////////////
// Global declarations, task handles

xTaskHandle handle_HeartbeatTask	= NULL;
xTaskHandle handle_CLITask		= NULL;
xTaskHandle handle_CmdTask		= NULL;

xTaskHandle handle_CommunicateTask = NULL;
xTaskHandle handle_CommunicateSendTask = NULL;
xTaskHandle handle_ControlLoopTask = NULL;
xTaskHandle handle_PressureTask = NULL;
xTaskHandle handle_AngleTask = NULL;
xTaskHandle handle_StatusLightTask = NULL;

SemaphoreHandle_t xControlLoopSemaphore = NULL;

void StartUserTasks(void);
void TaskControlLoop(void *pvParameters);
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

    #if (HARDWARE_CONNECTED == HARDWARE_TESTBOARD)
        SerialPrintf("> initializing hardware Testboard\n");
        io_Init();
        led_Init();
        button_Init();
        spi_Init();
        qc_Init();
        dac_Init();
        i2cOK  = i2c_Init();
        oledOK = oled_Init();
        uartOK = uart_Init();

    #elif (HARDWARE_CONNECTED == HARDWARE_HARROW)
        SerialPrintf("> initializing hardware Harrow\n");
        io_Init();
        led_Init();
        button_Init();
        spi_Init();
        qc_Init();
        dac_Init();
        i2cOK  = i2c_Init();
        oledOK = true; // oled_Init(); // NO OLED ON HARROW
        uartOK = uart_Init();

    #else
        SerialPrintf("> no hardware connected, skipping hardware initialization\n");
    #endif // HARDWARE_CONNECTED

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

    result &= platformTaskCreate(task_Heartbeat,      NULL, "task_heartbeat", &handle_HeartbeatTask);
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
	SerialPrintf("> RTSW_5_ButtonInterrupts\n");
	SerialPrintf("> build: %s\n", __TIMESTAMP__);
	SerialPrintf("> running setup\n");

    result = platformInit();

   	SerialPrintf("> setup done: %s\n", (result == true) ? "OK" : "FAILED");

#if (HARDWARE_CONNECTED == HARDWARE_TESTBOARD)    
        oled_Clear();
        oled_WriteLine(0, "RTSW",             ALIGN_CENTER);
        oled_WriteLine(1, "VKM PD",  ALIGN_CENTER);
        oled_WriteLine(2, "Wiedeg",          ALIGN_CENTER);
        oled_WriteLine(3, "V0.1",               ALIGN_CENTER);
#endif // HARDWARE_CONNECTED

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
// void StartUserTasks(void)

void StartUserTasks(void)
{
    BaseType_t result = pdFAIL;
    #if (HARDWARE_CONNECTED == HARDWARE_HARROW)
        SerialPrintf("> starting user tasks for Harrow\n");
    result &= platformTaskCreate(TaskCommunicate_Receive, NULL, "task_communicate_rx", &handle_CommunicateTask);
    result &= platformTaskCreate(TaskCommunicate_Send, NULL, "task_communicate_tx", &handle_CommunicateSendTask);
    result &= platformTaskCreate(TaskControlLoop, NULL, "task_control_loop", &handle_ControlLoopTask);
    result &= platformTaskCreate(MachineStatus, NULL, "task_status", &handle_StatusLightTask);
    #else
        SerialPrintf("> no user tasks defined for current hardware configuration\n");
    #endif // HARDWARE_CONNECTED
    xControlLoopSemaphore = xSemaphoreCreateBinary();
}

///////////////////////////////////////////////////////////////////////////////
//ISR

///////////////////////////////////////////////////////////////////////////////
// Funtions
void TaskControlLoop(void *pvParameters)
{
    BaseType_t result = pdFAIL;
    while (true)
    {
        if (xSemaphoreTake(xControlLoopSemaphore, portMAX_DELAY) == pdTRUE)
        {
            result &= platformTaskCreate(TaskPressure, NULL, "task_pressure", &handle_PressureTask);

            if (xSemaphoreTake(xControlLoopSemaphore, portMAX_DELAY) == pdTRUE)
            {
                vTaskDelete(&handle_PressureTask); // Delete pressure task to free resources
                result &= platformTaskCreate(TaskAngle, NULL, "task_angle", &handle_AngleTask);

                if (xSemaphoreTake(xControlLoopSemaphore, portMAX_DELAY) == pdTRUE)
                {
                    vTaskDelete(&handle_AngleTask); // Delete angle task to free resources
                    
                    // Control loop cycle complete, can add additional tasks or logic here
                }
                
            }
            
                // control loop code here
        }
        // control loop code here

        vTaskDelay(pdMS_TO_TICKS(100)); // delay to prevent watchdog reset
    }
}




///////////////////////////////////////////////////////////////////////////////
// void loop()

void loop()
{
    // totally empty!!!
}
