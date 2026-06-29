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

volatile bool estopActive = false;
volatile bool estopDone = false;

xTaskHandle handle_ControlLoopTask = NULL;
xTaskHandle handle_PressureTask = NULL;
xTaskHandle handle_AngleTask = NULL;
xTaskHandle handle_ESTOPHandlerTask = NULL;
xTaskHandle handle_ResetHandlerTask = NULL;

SemaphoreHandle_t xControlLoopSemaphore = NULL;
SemaphoreHandle_t xHandleStartControlLoop = NULL;
SemaphoreHandle_t xEstopSemaphore = NULL;
SemaphoreHandle_t xResetSemaphore = NULL;
SemaphoreHandle_t xDebugSemaphore = NULL;

bool motorinfoEnabled = false;
bool RealTimeModeEnabled = false;

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

static const uint32_t ESTOP_DEBOUNCE_MS = 250u;
void StartUserTasks(void);
void TaskControlLoop(void *pvParameters);
void TestTask(void *pvParameters);
void IRAM_ATTR buttonISR();
void ESTOPHandler(void *pvParameters);
void ResetHandler(void *pvParameters);
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
    //spi_Init();
    //qc_Init();
    //dac_Init();
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
    motorinfoEnabled = false; // Initialize motor info display flag to false
    RealTimeModeEnabled = false; // Initialize real-time mode flag to false

    SerialPrintf("> starting user tasks for Harrow\n");
    result &= platformTaskCreate(TaskControlLoop, NULL, "task_control_loop", &handle_ControlLoopTask);
    result &= platformTaskCreate(ESTOPHandler, NULL, "task_estop_handler", &handle_ESTOPHandlerTask);
    result &= platformTaskCreate(ResetHandler, NULL, "task_reset_handler", &handle_ResetHandlerTask);
    taskSleep(100); // Give the pin time to settle after startup

    if (digitalRead(PIN_BUTTON_ESTOP) == LOW)
    {
        SerialPrintf("> ESTOP input is already active at startup; waiting for release before arming interrupt\n");
        while (digitalRead(PIN_BUTTON_ESTOP) == LOW)
        {
            taskSleep(100);
        }
    }

    interrupt_AttachHandler(buttonISR, PIN_BUTTON_ESTOP, FALLING); // Attach button interrupt to ESTOP pin on falling edge
    SerialPrintf("> ESTOP interrupt armed\n");
    
    vTaskPrioritySet(handle_ESTOPHandlerTask, 10); // Set ESTOPHandlerTask to higher priority for testing purposes
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
        xSemaphoreTake(xHandleStartControlLoop, portMAX_DELAY);
        if (RealTimeModeEnabled)
        {
            // Currently crashes when both tasks are started simultaneously, so we will start them sequentially for now, (Possibly due to encoder handling, needs further investigation)
            SerialPrintf("> TaskControlLoop received start command...\n");
            SerialPrintf("> TaskControlLoop is running...\n");
            SerialPrintf("> Real-time mode enabled: pressure and angle control will run simultaneously\n");
            SerialPrintf("> Machine_Settings: IdealAngle = %d, IdealPressure = %.2f\n", Machine_Settings.IdealAngle, Machine_Settings.IdealPressure);
            taskSleep(MAX_MESSAGE_RATE_MS);

            if (handle_PressureTask == NULL){result &= platformTaskCreate(TaskPressure, NULL, "task_pressure", &handle_PressureTask);} // Start pressure task if not already running
            else{SerialPrintf("> Pressure task already running, skipping creation.\n");}
            if (handle_AngleTask == NULL){result &= platformTaskCreate(TaskAngle, NULL, "task_angle", &handle_AngleTask);} // Start angle task if not already running
            else{SerialPrintf("> Angle task already running, skipping creation.\n");}
            xSemaphoreTake(xControlLoopSemaphore, portMAX_DELAY); // Wait for both tasks to complete
            xSemaphoreTake(xControlLoopSemaphore, portMAX_DELAY); // Wait for both tasks to complete
        }
        else
        {
            SerialPrintf("> TaskControlLoop received start command...\n");
            SerialPrintf("> TaskControlLoop is running...\n");
            SerialPrintf("> Machine_Settings: IdealAngle = %d, IdealPressure = %.2f\n", Machine_Settings.IdealAngle, Machine_Settings.IdealPressure);
            taskSleep(MAX_MESSAGE_RATE_MS); // Allow time for settings to stabilize before starting tasks

            if (handle_PressureTask == NULL){result &= platformTaskCreate(TaskPressure, NULL, "task_pressure", &handle_PressureTask);}
            else{SerialPrintf("> Pressure task already running, skipping creation.\n");}
            xSemaphoreTake(xControlLoopSemaphore, portMAX_DELAY);
            if (handle_AngleTask == NULL){result &= platformTaskCreate(TaskAngle, NULL, "task_angle", &handle_AngleTask);}
            else{SerialPrintf("> Angle task already running, skipping creation.\n");}
            xSemaphoreTake(xControlLoopSemaphore, portMAX_DELAY);
        }
        SerialPrintf("> Machine reached ideal settings.\n");
    }
}

void IRAM_ATTR buttonISR()
{
    BaseType_t xHigherPriorityTaskWoken = pdFALSE;
    xSemaphoreGiveFromISR(xEstopSemaphore, &xHigherPriorityTaskWoken);
    portYIELD_FROM_ISR(xHigherPriorityTaskWoken);
}

void ESTOPHandler(void *pvParameters)
{
    uint8_t nEstopCount = 0;
    bool result = true;
    while (true)
    {
        xSemaphoreTake(xEstopSemaphore, portMAX_DELAY);
        if (estopActive) continue; // Ignore if already handling an ESTOP
        
        estopActive = true;
        estopDone   = false;

        nEstopCount++;
        SerialPrintf("> ESTOP button pressed! Initiating emergency stop...\n");
        SerialPrintf("> Total ESTOP presses: %d\n", nEstopCount);
        taskStatusLight(STATUS_ERROR_HARD);

        result = Estop_Brake();
        Estop_Pressure();
        Estop_Angle();

        if (handle_ControlLoopTask != NULL){vTaskSuspend(handle_ControlLoopTask);}  // Suspend the control loop task to prevent further motor commands
        if (handle_PressureTask != NULL){vTaskSuspend(handle_PressureTask);} // Suspend the pressure control task to stop motor operation
        if (handle_AngleTask != NULL){vTaskSuspend(handle_AngleTask);} // Suspend the angle control task to stop motor operation

        if (result) {SerialPrintf("> Emergency stop actions executed.\n");}
        else {SerialPrintf("> Estop actions failed, Please inform a supervisor"); while(1){};} // Suspend all further actions if Estop actions failed

        while (digitalRead(PIN_BUTTON_ESTOP) == LOW)
        {
            taskSleep(20);
        }
        estopDone   = true;
        SerialPrintf("> System is now in a safe state. Please reset using reset command to resume operation.\n");
        taskStatusLight(STATUS_ERROR_SOFT);
    }
}

void ResetHandler(void *pvParameters)
{
    while (true)
    {
        xSemaphoreTake(xResetSemaphore, portMAX_DELAY);
        SerialPrintf("> Resetting ESTOP...\n");
        if ((handle_ControlLoopTask != NULL) && (eTaskGetState(handle_ControlLoopTask) == eSuspended)) // Check if the control loop task exists and is suspended
        {vTaskResume(handle_ControlLoopTask);}  // Resume the control loop task

        if (Reset_Brake()) // Suspend all further actions if Brake failed
        {
            if ((handle_PressureTask != NULL) && (eTaskGetState(handle_PressureTask) == eSuspended)) // Check if the pressure control task exists and is suspended
            {vTaskResume(handle_PressureTask);} // Resume the pressure control task

            if ((handle_AngleTask != NULL)&&(eTaskGetState(handle_AngleTask) == eSuspended)) // Check if the angle control task exists and is suspended
            {vTaskResume(handle_AngleTask);} // Resume the angle control task

            estopActive = false; // Reset the ESTOP active flag to allow future ESTOP handling
            
            SerialPrintf("> ESTOP reset complete. System is now operational.\n");
        }
        else {SerialPrintf("> A Brake Failed to return to correct state, Please Restart the system using the restart command"); while(1){}} 
    }
}

void TestTask(void *pvParameters)
{
    while (true)
    {
        xSemaphoreTake(xHandleStartControlLoop, portMAX_DELAY);
        SerialPrintf("> TestTask is running...\n");
        SerialPrintf("> Machine_Settings: IdealAngle = %d, IdealPressure = %.2f\n", Machine_Settings.IdealAngle, Machine_Settings.IdealPressure);
        taskSleep(MAX_MESSAGE_RATE_MS); 
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
    pinMode(PIN_BUTTON_ESTOP, INPUT_PULLDOWN); // Initialize ESTOP button pin with internal pull-up
}

///////////////////////////////////////////////////////////////////////////////
// void loop()

void loop()
{
    // totally empty!!!
}
