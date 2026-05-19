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
#define PIN_TREE_RED              0
#define PIN_TREE_YELLOW           0
#define PIN_TREE_GREEN            0
#define PIN_TREE_BLINK            0 // IF LOW IT BLINKS, IF HIGH IT STAYS ON  (written before hardware was available, may need to be changed)

// Brake pins
#define PIN_BRAKE_UPPER_1         0 // IF LOW IT DISENGAGES THE BRAKE FOR M1 AND M2 (written before hardware was available, may need to be changed)
#define PIN_BRAKE_UPPER_2         0 // IF LOW IT DISENGAGES THE BRAKE FOR M1 AND M2 (written before hardware was available, may need to be changed)
#define PIN_BRAKE_UPPER_3         0 // IF LOW IT DISENGAGES THE BRAKE FOR M1 AND M2 (written before hardware was available, may need to be changed)
#define PIN_BRAKE_UPPER_4         0 // IF LOW IT DISENGAGES THE BRAKE FOR M1 AND M2 (written before hardware was available, may need to be changed)
#define PIN_BRAKE_LOWER           0 // IF LOW IT DISENGAGES THE BRAKE FOR ANGLE CHANGE (written before hardware was available, may need to be changed)

// Pressure pins
#define PIN_PRESSURE_MOTOR_PWM    0
#define PIN_PRESSURE_MOTOR_DIR    0 // IF LOW IT WINDS UP PRESSURE, IF HIGH IT RELEASES PRESSURE (written before hardware was available, may need to be changed)

#define PIN_PRESSURE_MOTOR_SEL_1  0 // IF LOW ENABLES M1 AND M2 (written before hardware was available, may need to be changed)
#define PIN_PRESSURE_MOTOR_SEL_2  0 // IF LOW ENABLES M3 AND M4 (written before hardware was available, may need to be changed)
#define PIN_PRESSURE_MOTOR_SEL_3  0 // IF LOW ENABLES M5 AND M6 (written before hardware was available, may need to be changed)
#define PIN_PRESSURE_MOTOR_SEL_4  0 // IF LOW ENABLES M7 AND M8 (written before hardware was available, may need to be changed)

#define PIN_PRESSURE_SENSOR_A     0 // HALL SENSOR PHASE A
#define PIN_PRESSURE_SENSOR_B     0 // HALL SENSOR PHASE B

// Agle Pins
#define PIN_ANGLE_MOTOR_PWM       0
#define PIN_ANGLE_MOTOR_DIR       0 // UNKNOWN CONTROL CONFIG (written before hardware was available, needs to be changed)

#define PIN_ANGLE_SENSOR_A        0 // HALL SENSOR PHASE A
#define PIN_ANGLE_SENSOR_B        0 // HALL SENSOR PHASE B
#define PIN_ANGLE_SENSOR_INDEX    0 // HALL SENSOR INDEX

#endif // HARDWARE_CONFIG_H