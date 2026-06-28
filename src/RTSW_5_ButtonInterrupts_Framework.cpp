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
//				12-06-2026 .... 19-06-2026 (Harrow V0.3) First Bugfix round after testing
//              24-06-2026 .... 26-06-2026 (Harrow V0.4) Second Bugfix round after testing on full harrow
//              28-06-2026 .... 29-06-2026 (Harrow V0.5) Final Bugfix round before submission
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


#include "Tasks_Custom/TaskBrakes.h" // implemented; verify hardware mapping
#include "Tasks_Custom/TaskCommunicate.h" // implemented
#include "Tasks_Custom/TaskPressure.h" // implemented
#include "Tasks_Custom/TaskAngle.h" // implemented (needs review)
#include "Tasks_Custom/TaskStatusLight.h"
#include "Tasks_Custom/MotorUtils.h"


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
xTaskHandle handle_TestTask = NULL;
xTaskHandle handle_ESTOPHandlerTask = NULL;

SemaphoreHandle_t xControlLoopSemaphore = NULL;
SemaphoreHandle_t xHandleStartControlLoop = NULL;
SemaphoreHandle_t xEstopSemaphore = NULL;
SemaphoreHandle_t xResetSemaphore = NULL;

CommunicationData_t Machine_Settings = {
    .IdealAngle = 0,
    .IdealPressure = 0.0f
};

typedef struct {
    gpio_num_t pin;
} IOPinConfig_t;

IOPinConfig_t ioPinConfigs[] = {
    {PIN_BRAKE_UPPER_1},
    {PIN_BRAKE_UPPER_2},
    {PIN_BRAKE_UPPER_3},
    {PIN_BRAKE_UPPER_4},
    {PIN_BRAKE_LOWER},
    {PIN_PRESSURE_MOTOR_DIR},
    {PIN_PRESSURE_MOTOR_SEL_1},
    {PIN_PRESSURE_MOTOR_SEL_2},
    {PIN_PRESSURE_MOTOR_SEL_3},
    {PIN_PRESSURE_MOTOR_SEL_4},
    {PIN_ANGLE_MOTOR_DIR},
};

uint8_t nEstopCount = 0;
void StartUserTasks(void);
void TaskControlLoop(void *pvParameters);
void TestTask(void *pvParameters);
void IRAM_ATTR buttonISR();
void ESTOPHandler(void *pvParameters);
void IO_INIT();

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

    SerialPrintf("> initializing hardware Testboard\n");
    //io_Init();
    led_Init();
    button_Init();
    spi_Init();
    qc_Init();
    dac_Init();
    IO_INIT();
    
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

    encoder_init();
    result = platformInit();

   	SerialPrintf("> setup done: %s\n", (result == true) ? "OK" : "FAILED");


    oled_Clear();
    oled_WriteLine(0, "RTSW",             ALIGN_CENTER);
    oled_WriteLine(1, "VKM PD",  ALIGN_CENTER);
    oled_WriteLine(2, "Wiedeg",          ALIGN_CENTER);
    oled_WriteLine(3, "V0.1",               ALIGN_CENTER);

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

    xControlLoopSemaphore = xSemaphoreCreateCounting(1, 0);
    xHandleStartControlLoop = xSemaphoreCreateCounting(1, 0);
    xEstopSemaphore = xSemaphoreCreateCounting(1, 0);
    xResetSemaphore = xSemaphoreCreateCounting(1, 0);
    xDebugSemaphore = xSemaphoreCreateCounting(1, 0);

    SerialPrintf("> starting user tasks for Harrow\n");
// CAN Functions, Not implemented yet, use cmd interface for now
//    result &= platformTaskCreate(TaskCommunicate_Receive, NULL, "task_communicate_rx", &handle_CommunicateTask);
//    result &= platformTaskCreate(TaskCommunicate_Send, NULL, "task_communicate_tx", &handle_CommunicateSendTask);
    result &= platformTaskCreate(TaskControlLoop, NULL, "task_control_loop", &handle_ControlLoopTask);
//    result &= platformTaskCreate(MachineStatus, NULL, "task_status", &handle_StatusLightTask);
//    result &= platformTaskCreate(TestTask, NULL, "TestTask", &handle_TestTask);
    result &= platformTaskCreate(ESTOPHandler, NULL, "task_estop_handler", &handle_ESTOPHandlerTask);
    pinMode(PIN_BUTTON_ESTOP, INPUT_PULLUP); // Initialize ESTOP button pin with internal pull-up
    interrupt_AttachHandler(buttonISR, PIN_BUTTON_ESTOP, FALLING); // Attach button interrupt to ESTOP pin on falling edge
//    interrupt_Enable(PIN_BUTTON_ESTOP); // Enable interrupt for ESTOP pin
    
    vTaskPrioritySet(handle_ESTOPHandlerTask, 3); // Set ESTOPHandlerTask to higher priority for testing purposes
}

///////////////////////////////////////////////////////////////////////////////
//ISR

///////////////////////////////////////////////////////////////////////////////
// Funtions
void TaskControlLoop(void *pvParameters)
{
    motorinfoEnabled = false; // Initialize motor info display flag to false
    RealTimeModeEnabled = false; // Initialize real-time mode flag to false
    BaseType_t result = pdFAIL;
    while (true)
    {
        xSemaphoreTake(xHandleStartControlLoop, portMAX_DELAY);
        if (RealTimeModeEnabled)
        {
            
            SerialPrintf("> TaskControlLoop received start command...\n");
            SerialPrintf("> TaskControlLoop is running...\n");
            SerialPrintf("> Real-time mode enabled: pressure and angle control will run simultaneously\n");
            SerialPrintf("> Machine_Settings: IdealAngle = %d, IdealPressure = %.2f\n", Machine_Settings.IdealAngle, Machine_Settings.IdealPressure);
            taskSleep(100);

            result &= platformTaskCreate(TaskPressure, NULL, "task_pressure", &handle_PressureTask);
            result &= platformTaskCreate(TaskAngle, NULL, "task_angle", &handle_AngleTask);
            xSemaphoreTake(xControlLoopSemaphore, portMAX_DELAY);
            xSemaphoreTake(xControlLoopSemaphore, portMAX_DELAY);
        }
        else
        {
            SerialPrintf("> TaskControlLoop received start command...\n");
            SerialPrintf("> TaskControlLoop is running...\n");
            SerialPrintf("> Machine_Settings: IdealAngle = %d, IdealPressure = %.2f\n", Machine_Settings.IdealAngle, Machine_Settings.IdealPressure);
            taskSleep(100);

            result &= platformTaskCreate(TaskPressure, NULL, "task_pressure", &handle_PressureTask);
            xSemaphoreTake(xControlLoopSemaphore, portMAX_DELAY);
            result &= platformTaskCreate(TaskAngle, NULL, "task_angle", &handle_AngleTask);
            xSemaphoreTake(xControlLoopSemaphore, portMAX_DELAY);
        }
    }
}

void IRAM_ATTR buttonISR()
{
    nEstopCount++;
    BaseType_t xHigherPriorityTaskWoken = pdFALSE;
    xSemaphoreGiveFromISR(xEstopSemaphore, &xHigherPriorityTaskWoken);
    portYIELD_FROM_ISR(xHigherPriorityTaskWoken);
}

void ESTOPHandler(void *pvParameters)
{
    while (true)
    {
        xSemaphoreTake(xEstopSemaphore, portMAX_DELAY);
            SerialPrintf("> ESTOP button pressed! Initiating emergency stop...\n");
            SerialPrintf("> Total ESTOP presses: %d\n", nEstopCount);
            taskStatusLight(STATUS_ERROR_HARD);
            Estop_Brake();
            if (handle_ControlLoopTask != NULL){vTaskDelete(handle_ControlLoopTask);}
            if (handle_PressureTask != NULL){vTaskDelete(handle_PressureTask);}
            if (handle_AngleTask != NULL){vTaskDelete(handle_AngleTask);}

            Estop_Pressure();
            Estop_Angle();
            SerialPrintf("> Emergency stop actions executed.\n");

            while (digitalRead(PIN_BUTTON_ESTOP) == LOW) // Wait until button is released
            {
                taskSleep(100); // Sleep to debounce and prevent busy-waiting
            //    SerialPrintf("> Current Button state = %d", digitalRead(PIN_BUTTON_ESTOP));
            }

            SerialPrintf("> System is now in a safe state. Please reset using reset command to resume operation.\n");
            taskStatusLight(STATUS_ERROR_SOFT);
            
            xSemaphoreTake(xResetSemaphore, portMAX_DELAY);
            
            SerialPrintf("> Reset signal received. Restarting system...\n");
            platformTaskCreate(TaskControlLoop, NULL, "task_control_loop", &handle_ControlLoopTask); // Restart control loop task
    }
}
void TestTask(void *pvParameters)
{
    while (true)
    {
        xSemaphoreTake(xHandleStartControlLoop, portMAX_DELAY);
        SerialPrintf("> TestTask is running...\n");
        SerialPrintf("> Machine_Settings: IdealAngle = %d, IdealPressure = %.2f\n", Machine_Settings.IdealAngle, Machine_Settings.IdealPressure);
        taskSleep(100);
    }
}

void IO_INIT()
{
    //while(true){
    for (int i = 0; i < (sizeof(ioPinConfigs) / sizeof(ioPinConfigs[0])); i++)
    {
        pinMode(ioPinConfigs[i].pin, OUTPUT); // Set all pins to output mode
        digitalWrite(ioPinConfigs[i].pin, HIGH); // Set all pins to high to prevent unintended outputs
    //    taskSleep(100); // Small delay to ensure proper initialization
    }
    /*
        for (int i = 0; i < (sizeof(ioPinConfigs) / sizeof(ioPinConfigs[0])); i++)
        {
            pinMode(ioPinConfigs[i].pin, OUTPUT); // Set all pins to output mode
            digitalWrite(ioPinConfigs[i].pin, LOW); // Set all pins to high to prevent unintended outputs
            taskSleep(100); // Small delay to ensure proper initialization
        }
    */
    //}

    pinMode(PIN_PRESSURE_MOTOR_PWM, OUTPUT); // Set PWM pin to analog mode for motor control
    analogWrite(PIN_PRESSURE_MOTOR_PWM, 0); // Initialize PWM output to 0 (motor off)

    pinMode(PIN_ANGLE_MOTOR_PWM, OUTPUT); // Set PWM pin to analog mode for motor control
    analogWrite(PIN_ANGLE_MOTOR_PWM, 0); // Initialize PWM output to 0 (motor off)

    // Initialize encoder pins with pull-up to prevent floating and noise issues
    pinMode(PIN_PRESSURE_SENSOR_A, INPUT_PULLUP);
    pinMode(PIN_PRESSURE_SENSOR_B, INPUT_PULLUP);
    pinMode(PIN_ANGLE_SENSOR_A, INPUT_PULLUP);
    pinMode(PIN_ANGLE_SENSOR_B, INPUT_PULLUP);
}

///////////////////////////////////////////////////////////////////////////////
// void loop()

void loop()
{
    // totally empty!!!
}
