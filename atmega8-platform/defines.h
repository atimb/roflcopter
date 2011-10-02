/**
 *  RoflCopter quadrotor project
 *  ATMEGA8 program source
 *
 *  (C) 2011 Attila Incze <attila.incze@gmail.com>
 *  http://atimb.me
 *
 *  This work is licensed under the Creative Commons Attribution-NonCommercial-ShareAlike 3.0 Unported License. To view a copy of this license, visit
 *  http://creativecommons.org/licenses/by-nc-sa/3.0/ or send a letter to Creative Commons, 444 Castro Street, Suite 900, Mountain View, California, 94041, USA.
 * 
 */

#ifndef UTIL_DEFINES_H
#define UTIL_DEFINES_H

#include "avr/io.h"

// The most important!
#define F_CPU 16000000UL
#define SYSCLK 16000000UL


// Other stuff
#define __ERR_LED PORTD|=_BV(PD5);
#define __OK_LED PORTD&=~_BV(PD5);
#define __FLASH_LED PORTD^=_BV(PD5);



// Control related defines
// Constants that need to be adjusted to actual gyro/accel/rx/engine/body/etc.

typedef struct {
	uint16_t RX_ROLLPITCH_MUL;
	uint16_t RX_YAW_MUL;
	uint16_t RX_THRUST_MUL;
	uint16_t GYRO_MUL;
	uint16_t ANGLE_ROLLPITCH_MUL;
	uint16_t ENGINE_MUL;
} settings_t;

typedef struct {
	int16_t gyro_data[3];
	int16_t acc_data[3];
	int16_t angle_data[2];
	int16_t control[4];
	uint8_t engines[4];
	uint8_t esc_status;
} controls_t;

extern volatile uint8_t copter_state;

extern volatile settings_t settings;
extern volatile controls_t controls;

extern volatile uint8_t motor_errors[4];

extern volatile uint8_t copter_state;

extern volatile uint8_t rx_new_data_avail;

extern volatile uint16_t rx_start_position[8];
extern volatile uint16_t rx_verified_data[8];

extern volatile uint16_t adc_gyro_data[3];

extern volatile uint8_t gyro_compensation_enabled;
extern volatile uint8_t acc_compensation_enabled;

enum {
	ACC_X,
	ACC_Y,
	ACC_Z
};

enum {
	GYRO_YAW,
	GYRO_PITCH,
	GYRO_ROLL
};

enum {
	RX_YAW,
	RX_PITCH,
	RX_THRUST,
	RX_ROLL
};

enum {
	CTRL_YAW,
	CTRL_ROLL,
	CTRL_PITCH,
	CTRL_THRUST
};

enum {
	ANGLE_ROLL,
	ANGLE_PITCH
};

enum {
	ENGINE_FRONT,
	ENGINE_REAR,
	ENGINE_LEFT,
	ENGINE_RIGHT
};


// TWI defines
#define TWI_LIS3LV          0x3A   // TWI address for LIS3LV02DQ sensor
#define TWI_LIS3LV_CONTROL  0x20   // TWI register addr for acceleration sensor setup
#define TWI_LIS3LV_CTRL_VAL 0xC7   // Value for previous field
#define TWI_LIS3LV_OUT      0x28   // TWI register addr for acceleration values
#define TWI_MOTOR_1         0x52   // TWI address for brushless ctrler #1
#define MAX_TWI_ITER	    200    // TWI communication timeout


// USART defines
#define USART_BAUDRATE 19200 
#define BAUD_PRESCALE (((F_CPU / (USART_BAUDRATE * 16UL))) - 1) 



#endif
