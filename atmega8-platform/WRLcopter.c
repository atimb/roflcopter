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

#include "defines.h"
#include "avr/io.h"
#include "util/delay.h"
#include "avr/interrupt.h"
#include "avr/eeprom.h"
#include "util_twi.h"
#include "util_usart.h"
#include "rc_rx.h"
#include "math.h"
#include "TWI_Master.h"
#include "adc.h"

unsigned char twi_messageBuf[4];
uint8_t motor_send_index = 0;

volatile settings_t settings;
volatile controls_t controls;

volatile uint8_t motor_errors[4];

volatile uint8_t copter_state = 1;
volatile uint16_t rx_start_position[8];
volatile uint16_t adc_gyro_start_position[3];

volatile uint8_t twi_state = 1;

volatile uint16_t adc_gyro_data[3];

volatile uint8_t gyro_compensation_enabled = 1;

void business_logics() {

	/* GYRO COMPENSATION */
	for (int i=0; i<3; ++i)  {
		controls.gyro_data[i] = (int16_t)adc_gyro_data[i] - (int16_t)adc_gyro_start_position[i];
	}

	/* REMOTE CONTROLLER */
	controls.control[CTRL_THRUST] = (int16_t)rx_verified_data[RX_THRUST] - (int16_t)rx_start_position[RX_THRUST];
	controls.control[CTRL_ROLL] = (int16_t)rx_verified_data[RX_ROLL] - (int16_t)rx_start_position[RX_ROLL];
	controls.control[CTRL_PITCH] = (int16_t)rx_verified_data[RX_PITCH] - (int16_t)rx_start_position[RX_PITCH];
	controls.control[CTRL_YAW] = (int16_t)- rx_verified_data[RX_YAW] + (int16_t)rx_start_position[RX_YAW];

	if (gyro_compensation_enabled) {
	    controls.control[CTRL_ROLL] += controls.gyro_data[GYRO_ROLL]*2;
		controls.control[CTRL_PITCH] -= controls.gyro_data[GYRO_PITCH]*2;
	    controls.control[CTRL_YAW] -= controls.gyro_data[GYRO_YAW]*5;
    }
	
	if (controls.control[CTRL_THRUST] < 0)
		controls.control[CTRL_THRUST] = 0;
	if (controls.control[CTRL_THRUST] > 750)
		controls.control[CTRL_THRUST] = 750;
	
	if (controls.control[CTRL_ROLL] < -375)
		controls.control[CTRL_ROLL] = -375;
	if (controls.control[CTRL_ROLL] > 375)
		controls.control[CTRL_ROLL] = 375;

	if (controls.control[CTRL_PITCH] < -375)
		controls.control[CTRL_PITCH] = -375;
	if (controls.control[CTRL_PITCH] > 375)
		controls.control[CTRL_PITCH] = 375;

	if (controls.control[CTRL_YAW] < -375)
		controls.control[CTRL_YAW] = -375;
	if (controls.control[CTRL_YAW] > 375)
		controls.control[CTRL_YAW] = 375;

	/* CALCULATE ENGINE THRUST */
	int16_t tempengines[4];
	tempengines[ENGINE_FRONT] = controls.control[CTRL_THRUST] / 3;
	tempengines[ENGINE_REAR] = controls.control[CTRL_THRUST] / 3;
	tempengines[ENGINE_LEFT] = controls.control[CTRL_THRUST] / 3;
	tempengines[ENGINE_RIGHT] = controls.control[CTRL_THRUST] / 3;

	tempengines[ENGINE_LEFT] += controls.control[CTRL_ROLL] >> 2;
	tempengines[ENGINE_RIGHT] -= controls.control[CTRL_ROLL] >> 2;

	tempengines[ENGINE_FRONT] -= controls.control[CTRL_PITCH] >> 2;
	tempengines[ENGINE_REAR] += controls.control[CTRL_PITCH] >> 2;

	tempengines[ENGINE_FRONT] -= controls.control[CTRL_YAW] / 6;
	tempengines[ENGINE_REAR] -= controls.control[CTRL_YAW] / 6;
	tempengines[ENGINE_LEFT] += controls.control[CTRL_YAW] / 6;
	tempengines[ENGINE_RIGHT] += controls.control[CTRL_YAW] / 6;

	/* LOWFASZ FILTER */
	for (int i=0; i<4; i++) {
		tempengines[i] += controls.engines[i];
		tempengines[i] = tempengines[i] >> 1;
	}

	for (int i=0; i<4; i++) {
		if (tempengines[i] > 150)
			controls.engines[i] = 150;
		else if (tempengines[i] < 0)
			controls.engines[i] = 0;
		else
			controls.engines[i] = tempengines[i];
	}

	/* FAILSAFE: IF THROTTLE IS VERY LOW, STOP ALL MOTORS */
	if (controls.control[CTRL_THRUST] < 50) {
		for (int i=0; i<4; i++) {
			controls.engines[i] = 0;
		}
	}
}

void setup_AccSensor() {
	USART_send_byte(0xFA);
	do {
		if (!TWI_Transceiver_Busy()) {
			switch (twi_state++) {
				case 1:
					TWI_statusReg.lastTransOK = FALSE;
					twi_messageBuf[0] = TWI_LIS3LV;
					twi_messageBuf[1] = 0x20;
					TWI_Start_Transceiver_With_Data( twi_messageBuf, 2);
					USART_send_byte(0xFF);
					break;
				case 2:
					TWI_statusReg.lastTransOK = FALSE;
					twi_messageBuf[0] = TWI_LIS3LV | 0x01; // READ BIT
					TWI_Start_Transceiver_With_Data( twi_messageBuf, 1);
					USART_send_byte(0xFE);
					break;
				case 3:
					TWI_statusReg.lastTransOK = FALSE;
					TWI_Get_Data_From_Transceiver( twi_messageBuf, 1);
					USART_send_byte(0xFD);
					USART_send_byte(twi_messageBuf[0]);
					//twi_messageBuf[1] = twi_messageBuf[0] | 0xC0;	// Add start sensor flags
					//twi_messageBuf[0] = TWI_LIS3LV;
					//TWI_Start_Transceiver_With_Data( twi_messageBuf, 2);
					break;
			}
		}
	} while (twi_state < 4);
	twi_state = 1;
}

void sendToMotor() {
	_delay_ms(1);  // fix for TWI block bug
	usart_process();
	_delay_ms(1);  // fix for TWI block bug
	usart_process();
	_delay_ms(1);  // fix for TWI block bug
	usart_process();
	_delay_ms(1);  // fix for TWI block bug
	usart_process();
	_delay_ms(1);  // fix for TWI block bug
	usart_process();
	if (!TWI_Transceiver_Busy()) {
		switch (twi_state++) {
			case 1:
			case 2:
			case 3:
			case 4:
				// Check if the last operation was faulty
		  		if (!TWI_statusReg.lastTransOK) {
					if (++motor_errors[motor_send_index] == 0) motor_errors[motor_send_index]=255;
				} else {
					motor_errors[motor_send_index] = 0;
				}
				TWI_statusReg.lastTransOK = FALSE;
				// Increase index
				if (motor_send_index==3) {
					motor_send_index = 0;
				} else {
					motor_send_index++;
				}
				// Send next motor status
				twi_messageBuf[0] = 0x52+motor_send_index*2;
				twi_messageBuf[1] = controls.engines[motor_send_index];
				TWI_Start_Transceiver_With_Data( twi_messageBuf, 2);
				break;
			case 5:
				twi_messageBuf[0] = TWI_LIS3LV;
				twi_messageBuf[1] = 0x28;
				TWI_Start_Transceiver_With_Data( twi_messageBuf, 2);
				break;
			case 6:
				twi_messageBuf[0] = TWI_LIS3LV | 0x01;	// READ BIT
				TWI_Start_Transceiver_With_Data( twi_messageBuf, 6);
				break;
			case 7:
				TWI_Get_Data_From_Transceiver( twi_messageBuf, 6 );
				for (int i=0; i<3; ++i) {
					controls.acc_data[i] = twi_messageBuf[2*i] | twi_messageBuf[2*i+1]<<8;
				}
				twi_state = 1;
				break;
		}
	}
}


int main() {

	DDRD = _BV(PD5);	// Blue status LED
	PORTD = _BV(PD2);	// Interal pull-up

	for (uint8_t i=0; i<3; ++i) {	// Flash LED, still alive!
		__ERR_LED
		_delay_ms(50);
		__OK_LED
		_delay_ms(100);
	}

	setup_USART();
	setup_RX();
	setup_ADC();

	TWI_Master_Initialise();

	sei();

	setup_AccSensor();

	for (uint8_t i=0; i<4; ++i) {
		motor_errors[i] = 0;
	}

	uint32_t rx_signal_sum[8];
	for (uint8_t i=0; i<8; ++i) {
		rx_signal_sum[i] = 0;
	}

	uint32_t adc_gyro_sum[3];
	for (uint8_t i=0; i<3; ++i) {
		adc_gyro_sum[i] = 0;
	}

	uint8_t rx_sample_count = 0;
	
    gyro_compensation_enabled = 1;

	__ERR_LED;


	while (1) {		// Main cycle

		usart_process();
		
		__FLASH_LED;

		switch (copter_state) {
			case 1:
				if (rx_new_data_avail) {
					rx_new_data_avail = 0;
					for (uint8_t i=0; i<8; ++i) {
						rx_signal_sum[i] += rx_verified_data[i];
					}
					for (uint8_t i=0; i<3; ++i) {
						adc_gyro_sum[i] += adc_gyro_data[i];
					}
					if (rx_sample_count++ == 63) {
						copter_state = 10;
						for (uint8_t i=0; i<8; ++i) {
							rx_start_position[i] = rx_signal_sum[i]>>6;	// %64
						}
						for (uint8_t i=0; i<3; ++i) {
							adc_gyro_start_position[i] = adc_gyro_sum[i]>>6;	// %64
						}
					}
				}
				break;
			case 10:	// Ready To Fly
				business_logics();
				sendToMotor();
				break;
			case 99:	// Panic mode!
				for (int i=0; i<4; i++) {
					controls.engines[i] = 0;
				}
				sendToMotor();
				break;
		}
	
	}


}

