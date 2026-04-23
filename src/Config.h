///////////////////////////////////////////////////////////////////////////////
//
// Config.h
//
// Authors: 	Roel Smeets
// Edit date: 	02-06-2025
//
///////////////////////////////////////////////////////////////////////////////

#ifndef CONFIG_H
#define CONFIG_H

///////////////////////////////////////////////////////////////////////////////
// define during development for testing individual devices on the board,
// 0 = regular operation, 1 = systems test only

#define SYSTEMTEST_ONLY		0

///////////////////////////////////////////////////////////////////////////////
// DAC constants for MCP4922 & output stage

#define N_DAC_BITS			12
#define N_DAC_CHANNELS		4

#define DAC_MIN_CHANNEL		0 
#define DAC_MAX_CHANNEL		(N_DAC_CHANNELS - 1)

#define DAC_MIN_VALUE 		0
#define DAC_MAX_VALUE 		((1 << N_DAC_BITS) - 1)

#define DAC_SPAN            20.0    // -10 .. + 10 volt
#define DAC_RESOLUTION	    (DAC_SPAN / (DAC_MAX_VALUE + 1))

#define DAC_MIN_VOLTAGE 	-10.0
#define DAC_MAX_VOLTAGE 	(DAC_MIN_VOLTAGE + ((DAC_MAX_VALUE) * (DAC_RESOLUTION)))


///////////////////////////////////////////////////////////////////////////////
// ADC constants & definitions for MCP3208 8 channel ADC

#define N_ADC_CHANNELS      8
#define N_ADC_BITS		    12

#define ADC_MIN_CHANNEL		0
#define ADC_MAX_CHANNEL		(N_ADC_CHANNELS - 1)

#define ADC_MIN_VALUE 	    0
#define ADC_MAX_VALUE       ((1 << N_ADC_BITS) - 1)

#define ADC_REFERENCE_VOLTAGE   2.5

// definitions for channels 0..3 with input range from -10 .. +10 volt

#define ADC03_MIN_VOLTAGE       -10.0
#define ADC03_RESOLUTION        ((8.0 * ADC_REFERENCE_VOLTAGE) / (ADC_MAX_VALUE + 1))
#define ADC03_MAX_VOLTAGE		(ADC03_MIN_VOLTAGE + ((ADC_MAX_VALUE) * (ADC03_RESOLUTION)))

// definitions for channels 4..7 with input range from 0 .. +2.5 volt

#define ADC47_RESOLUTION        (ADC_REFERENCE_VOLTAGE / (ADC_MAX_VALUE + 1))
#define ADC47_MIN_VOLTAGE	    0.0
#define ADC47_MAX_VOLTAGE		((ADC_MAX_VALUE) * (ADC47_RESOLUTION))

///////////////////////////////////////////////////////////////////////////////
// conversion factors

#define VOLT_TO_MV			(1e3)
#define MV_TO_VOLT			(1e-3)

///////////////////////////////////////////////////////////////////////////////
// RTOS defines for xTaskCreatePinnedToCore

#define	RTOS_DEFAULT_STACKSIZE	4096    // RTOS task stack size in bytes
#define CORE_0	    0                   // pin to core 0 of ESP32, Core 0 = Protocol CPU (WiFi/BT Stack)
#define CORE_1	    1                   // pin to core 1 of ESP32, user apps

///////////////////////////////////////////////////////////////////////////////
// I2C address definitions

#define I2C_ADDRESS_OLED		0x3C	    // I2C address of OLED display
#define I2C_ADDRESS_UART        0x4D        // I2C address of UART 
#define I2C_ADDRESS_EEPROM		0x50		// I2C address of the external (!!) EEPROM on the I2C bus

#endif	// CONFIG_H
