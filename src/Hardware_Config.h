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

#define HARDWARE_NONE 0
#define HARDWARE_TESTBOARD 1
#define HARDWARE_HARROW 2

#define HARDWARE_CONNECTED HARDWARE_HARROW // Set to true when hardware is available and pins are defined

#if (HARDWARE_CONNECTED == HARDWARE_HARROW)

    // Comunication pins
    #define PIN_TX                    GPIO_NUM_1
    #define PIN_RX                    GPIO_NUM_2

    // Christmas tree pins (Signal Lights)
    #define PIN_TREE_RED              GPIO_NUM_3
    #define PIN_TREE_YELLOW           GPIO_NUM_4
    #define PIN_TREE_GREEN            GPIO_NUM_5
    #define PIN_TREE_BLINK            GPIO_NUM_6 // IF LOW IT BLINKS, IF HIGH IT STAYS ON  (written before hardware was available, may need to be changed)

    // Brake pins
    #define PIN_BRAKE_UPPER_1         GPIO_NUM_7 // IF LOW IT DISENGAGES THE BRAKE FOR M1 AND M2 (written before hardware was available, may need to be changed)
    #define PIN_BRAKE_UPPER_2         GPIO_NUM_8 // IF LOW IT DISENGAGES THE BRAKE FOR M1 AND M2 (written before hardware was available, may need to be changed)
    #define PIN_BRAKE_UPPER_3         GPIO_NUM_9 // IF LOW IT DISENGAGES THE BRAKE FOR M1 AND M2 (written before hardware was available, may need to be changed)
    #define PIN_BRAKE_UPPER_4         GPIO_NUM_10 // IF LOW IT DISENGAGES THE BRAKE FOR M1 AND M2 (written before hardware was available, may need to be changed)
    #define PIN_BRAKE_LOWER           GPIO_NUM_11 // IF LOW IT DISENGAGES THE BRAKE FOR ANGLE CHANGE (written before hardware was available, may need to be changed)

    // Pressure pins
    #define PIN_PRESSURE_MOTOR_PWM    GPIO_NUM_12
    #define PIN_PRESSURE_MOTOR_DIR    GPIO_NUM_13 // IF LOW IT WINDS UP PRESSURE, IF HIGH IT RELEASES PRESSURE (written before hardware was available, may need to be changed)

    #define PIN_PRESSURE_MOTOR_SEL_1  GPIO_NUM_14 // IF LOW ENABLES M1 AND M2 (written before hardware was available, may need to be changed)
    #define PIN_PRESSURE_MOTOR_SEL_2  GPIO_NUM_15 // IF LOW ENABLES M3 AND M4 (written before hardware was available, may need to be changed)
    #define PIN_PRESSURE_MOTOR_SEL_3  GPIO_NUM_16 // IF LOW ENABLES M5 AND M6 (written before hardware was available, may need to be changed)
    #define PIN_PRESSURE_MOTOR_SEL_4  GPIO_NUM_17 // IF LOW ENABLES M7 AND M8 (written before hardware was available, may need to be changed)

    #define PIN_PRESSURE_SENSOR_A     GPIO_NUM_18 // HALL SENSOR PHASE A
    #define PIN_PRESSURE_SENSOR_B     GPIO_NUM_19 // HALL SENSOR PHASE B

    // Agle Pins
    #define PIN_ANGLE_MOTOR_PWM       GPIO_NUM_20
    #define PIN_ANGLE_MOTOR_DIR       GPIO_NUM_21 // UNKNOWN CONTROL CONFIG (written before hardware was available, needs to be changed)

    #define PIN_ANGLE_SENSOR_A        GPIO_NUM_22 // HALL SENSOR PHASE A
    #define PIN_ANGLE_SENSOR_B        GPIO_NUM_23 // HALL SENSOR PHASE B
    #define PIN_ANGLE_SENSOR_INDEX    GPIO_NUM_24 // HALL SENSOR INDEX

#endif // HARDWARE_CONNECTED

#endif // HARDWARE_CONFIG_H