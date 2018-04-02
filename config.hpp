#pragma once
#ifndef SOAR_CONFIG_HPP
#define SOAR_CONFIG_HPP

/*-----------------------------
* Task Priorities
* 5: highest
* 1: lowest
*----------------------------*/
#define AHRS_UPDATE_PRIORITY		3
#define STATUS_LEDS_PRIORITY		1
#define CONSOLE_LOGGING_PRIORITY	2


/*-----------------------------
* Task Update Frequencies/Constants
*----------------------------*/
#define STATUS_LED_UPDATE_FREQ_HZ	5		
#define CONSOLE_UPDATE_FREQ_HZ		100	
#define SENSOR_UPDATE_FREQ_HZ		250		
#define AHRS_UPDATE_RATE_MULTIPLIER	5		/* AHRS will have an effective update at X multiple of SENSOR_UPDATE_FREQ_HZ (x5) */

/*-----------------------------
* Memory Management
*----------------------------*/
#define QUEUE_MINIMUM_SIZE			5

#endif 