## Author
This directory contain tasks created solely by student Oliver Hofman.

## Tasks
The 5 main software tasks are:
* TaskCommunicate
* TaskBrakes
* TaskStatusLight
* TaskPressure
* TaskAngle

These are described within this README file and their respective .cpp and .h files

## TaskCommunicate
A task built to handle serial bus comunication. It contains functions for transmitting, recieving and processing data.

## TaskBrakes
Controls the brake system of the harrow by managing brake engagement and disengagement. The task accepts a boolean state (BrakeOn) and a brake id number as parameters. When BrakeOn is true, the brake pin is driven high to engage the brakes; when false, the pin is driven low to disengage them. The function returns a status boolean for error handling, allowing the calling routine to detect any brake control failures and trigger appropriate error responses.

## TaskStatusLight
Provides visual system status indication through an RGB LED signal tower with multiple operational states. The task accepts a status code and drives the red, green, and yellow LEDs accordingly, along with a blink control pin. It supports ten distinct status modes:
* STATUS_NOCONTROL (yellow blinking) - System initializing
* STATUS_ERROR_HARD (red blinking) - Critical error requiring immediate attention 
* STATUS_ERROR_SOFT (red solid) - Non-critical error detected
* STATUS_ALLGOOD (green solid) - System ready and idle
* STATUS_STANDBY (yellow solid) - System in standby mode
* STATUS_RUNNING (green blinking) - System actively operating
* STATUS_WARNING (red + yellow solid) - Warning condition detected
* STATUS_DISABLED (all LEDs off) - System offline or disabled
* STATUS_MAINTENANCE (all colors blinking) - Diagnostic or service mode in progress
* STATUS_WORKING (green + yellow blinking) - Adjustments in progress (Stay clear of harrow)

This comprehensive state feedback allows operators to quickly assess machine health, operational readiness, and current activity at a glance.

## TaskPressure

## TaskAngle

