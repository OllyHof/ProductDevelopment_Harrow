///////////////////////////////////////////////////////////////////////////////
//
// Hardware_Config.h
//
// Authors: 	Oliver Hofman
// Edit date: 	19-05-2026
//
///////////////////////////////////////////////////////////////////////////////


///////////////////////////////////////////////////////////////////////////////
// system #includes

#include <Arduino.h>

///////////////////////////////////////////////////////////////////////////////

#ifndef HARDWARE_CONFIG_H
#define HARDWARE_CONFIG_H
    // Comunication pins
    #define PIN_TX                    0
    #define PIN_RX                    0

    // Christmas tree pins (Signal Lights)
    /* Section commented out since it won't be used in the current machine configuration
    #define PIN_TREE_RED              0 // - NOT USED IN CURRENT CONFIG
    #define PIN_TREE_YELLOW           0 // - NOT USED IN CURRENT CONFIG
    #define PIN_TREE_GREEN            0 // - NOT USED IN CURRENT CONFIG
    #define PIN_TREE_BLINK            0 // IF LOW IT BLINKS, IF HIGH IT STAYS ON  (written before hardware was available, may need to be changed)
    */

    // Brake pins
    #define PIN_BRAKE_UPPER_1         GPIO_NUM_2 // IF LOW IT DISENGAGES THE BRAKE FOR M1 AND M2 (written before hardware was available, may need to be changed)
	/* Section commented out since it won't be used in the current machine configuration
    #define PIN_BRAKE_UPPER_2         GPIO_NUM_4 // IF LOW IT DISENGAGES THE BRAKE FOR M1 AND M2 (written before hardware was available, may need to be changed)
    #define PIN_BRAKE_UPPER_3         GPIO_NUM_5 // IF LOW IT DISENGAGES THE BRAKE FOR M1 AND M2 (written before hardware was available, may need to be changed)
    #define PIN_BRAKE_UPPER_4         GPIO_NUM_19 // IF LOW IT DISENGAGES THE BRAKE FOR M1 AND M2 (written before hardware was available, may need to be changed)
    #define PIN_BRAKE_LOWER           GPIO_NUM_21 // IF LOW IT DISENGAGES THE BRAKE FOR ANGLE CHANGE (written before hardware was available, may need to be changed)
    */
    // Pressure pins
    #define PIN_PRESSURE_MOTOR_PWM    GPIO_NUM_22
    #define PIN_PRESSURE_MOTOR_DIR    GPIO_NUM_23 // IF LOW IT WINDS UP PRESSURE, IF HIGH IT RELEASES PRESSURE (written before hardware was available, may need to be changed)

    #define PIN_PRESSURE_MOTOR_SEL_1  GPIO_NUM_13 // IF LOW ENABLES M1 AND M2 (written before hardware was available, may need to be changed)
	/* Section commented out since it won't be used in the current machine configuration
    #define PIN_PRESSURE_MOTOR_SEL_2  GPIO_NUM_12 // IF LOW ENABLES M3 AND M4 (written before hardware was available, may need to be changed)
    #define PIN_PRESSURE_MOTOR_SEL_3  GPIO_NUM_14 // IF LOW ENABLES M5 AND M6 (written before hardware was available, may need to be changed)
    #define PIN_PRESSURE_MOTOR_SEL_4  GPIO_NUM_27 // IF LOW ENABLES M7 AND M8 (written before hardware was available, may need to be changed)
    */    
    #define PIN_PRESSURE_SENSOR_A     GPIO_NUM_26 // HALL SENSOR PHASE A
    #define PIN_PRESSURE_SENSOR_B     GPIO_NUM_25 // HALL SENSOR PHASE B

    // Angle Pins
    #define PIN_ANGLE_MOTOR_PWM       GPIO_NUM_33
    #define PIN_ANGLE_MOTOR_DIR       GPIO_NUM_32 // UNKNOWN CONTROL CONFIG (written before hardware was available, needs to be changed)

    #define PIN_ANGLE_SENSOR_A        GPIO_NUM_34 // HALL SENSOR PHASE A
    #define PIN_ANGLE_SENSOR_B        GPIO_NUM_35 // HALL SENSOR PHASE B
    #define PIN_ANGLE_SENSOR_INDEX    0 // HALL SENSOR INDEX  - NOT USED IN CURRENT CONFIG

    // Button Pins
    #define PIN_BUTTON_ESTOP          GPIO_NUM_18 // IF LOW IT TRIGGERS THE ESTOP (written before hardware was available, may need to be changed)

#endif // HARDWARE_CONFIG_H;