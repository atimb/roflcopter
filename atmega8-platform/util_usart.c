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

#include "util_usart.h"
#include "avr/io.h"
#include "avr/interrupt.h"
#include "util/delay.h"
#include "defines.h"
#include "adc.h"


/*------------------------------*/
/*        USART ASYNC RECV      */
/*------------------------------*/

#define USART_SYNC_BYTE 0xAA
#define USART_SYNC_LENGTH 4
#define USART_BUFFER_SIZE 10
#define USART_MIN_PACKET_SIZE 3

enum USART_states {
	USART_OUT_OF_SYNC,
	USART_RECEIVING
};

volatile uint8_t usart_buffer[USART_BUFFER_SIZE];
volatile uint8_t usart_buffer_index = 0;

volatile uint8_t usart_sync_bytes = 0;
volatile uint8_t usart_state = USART_OUT_OF_SYNC;



ISR(USART_RXC_vect) {
	// read it out!
	uint8_t data = UDR;

	if (data == USART_SYNC_BYTE) {
		usart_sync_bytes++;
		if (usart_sync_bytes >= USART_SYNC_LENGTH) {
			usart_state = USART_RECEIVING;
			usart_buffer_index = 0;
			return;
		}
	} else {
		usart_sync_bytes = 0;
	}

	if (usart_state == USART_RECEIVING) {
		if (usart_buffer_index == USART_BUFFER_SIZE) {
			usart_state = USART_OUT_OF_SYNC;
			usart_buffer_index = 0;
			return;
		}
		usart_buffer[usart_buffer_index++] = data;
	}

}


/*------------------------------*/
/*        USART ASYNC SEND      */
/*------------------------------*/

#define USART_SENDBUFFER_SIZE 10

volatile uint8_t usart_sendbuffer[USART_SENDBUFFER_SIZE];
volatile uint8_t usart_sendbuffer_index = 0;
volatile uint8_t usart_sendlength = 0;


ISR(USART_TXC_vect) {
	if (usart_sendbuffer_index == usart_sendlength)	// Ready with sending all bytes
		return;
	UDR = usart_sendbuffer[usart_sendbuffer_index++];	// Send next byte
}


void USART_async_send_start(uint8_t length) {
	usart_sendlength = length;
	usart_sendbuffer_index = 1;
	UDR = usart_sendbuffer[0];
}


/*------------------------------*/
/*        USART SYNC PROCESS    */
/*------------------------------*/

void usart_process() {

	if (usart_buffer_index < USART_MIN_PACKET_SIZE)	// Minimum packet length
		return;

	if (usart_buffer_index != usart_buffer[1])	// Whole package arrived
		return;

	usart_buffer_index = 0;

	uint8_t checksum = 0;
	for (uint8_t i=0; i<usart_buffer[1]; ++i) {		// Calculate checksum
		checksum += usart_buffer[i];
	}

	if (checksum != 0) { 	// Faulty transmission
		usart_state = USART_OUT_OF_SYNC;
		return;
	}

	switch (usart_buffer[0]) {	// message ID
		case 0x00:  handleResetCommand(); break;
		case 0x01:	handlePingCommand(); break;
		case 0x02:  handleRxCommand(); break;
		case 0x03:  handleMotorErrorCommand(); break;
		case 0x04:  handleRxStartPosCommand(); break;
		case 0x05:  handleCopterStateCommand(); break;
		case 0x06:  handleMotorCommand(); break;
		case 0x07:  handleRxControlCommand(); break;
		case 0x08:  handleGyroCommand(); break;
		case 0x09:  handleGyroControlCommand(); break;
		case 0x0A:  handlePanicCommand(); break;
		default: break;
	}

}


void handlePingCommand() {
	usart_sendbuffer[0] = 0x55;
	usart_sendbuffer[1] = 0x99;
	usart_sendbuffer[2] = 0xEE;
	USART_async_send_start(3);
}

void handleRxStartPosCommand() {
	for (uint8_t i=0; i<4; i++) {
		usart_sendbuffer[2*i] = rx_start_position[i]>>8;
		usart_sendbuffer[2*i+1] = rx_start_position[i];
	}
	usart_sendbuffer[8] = 0xCC;
	USART_async_send_start(9);
}

void handleRxCommand() {
	for (uint8_t i=0; i<4; i++) {
		usart_sendbuffer[2*i] = rx_verified_data[i]>>8;
		usart_sendbuffer[2*i+1] = rx_verified_data[i];
	}
	usart_sendbuffer[8] = 0xBA;
	USART_async_send_start(9);
}


void handleRxControlCommand() {
	for (uint8_t i=0; i<4; i++) {
		usart_sendbuffer[2*i] = (uint16_t)(1500+controls.control[i])>>8;
		usart_sendbuffer[2*i+1] = (uint16_t)(1500+controls.control[i]);
	}
	usart_sendbuffer[8] = 0xBA;
	USART_async_send_start(9);
}

void handleResetCommand() {
	// reset ourselves using watchdog
	USART_send_byte(0x00);	// ACK!
	cli();
	WDTCR = _BV(WDE);
	_delay_ms(100);
}

void handleMotorErrorCommand() {
	for (uint8_t i=0; i<4; i++) {
		usart_sendbuffer[i] = motor_errors[i];
	}
	usart_sendbuffer[4] = 0xBA;
	USART_async_send_start(5);
}

void handleCopterStateCommand() {
	usart_sendbuffer[0] = copter_state;
	usart_sendbuffer[1] = 0xEE;
	USART_async_send_start(2);
}

void handleMotorCommand() {
	for (uint8_t i=0; i<4; i++) {
		usart_sendbuffer[i] = controls.engines[i];
	}
	usart_sendbuffer[4] = 0x66;
	USART_async_send_start(5);
}

void handleGyroCommand() {
	for (uint8_t i=0; i<3; i++) {
		usart_sendbuffer[2*i] = adc_gyro_data[i]>>8;
		usart_sendbuffer[2*i+1] = adc_gyro_data[i];
	}
	usart_sendbuffer[6] = 0xAA;
	USART_async_send_start(7);
}

void handleGyroControlCommand() {
	for (uint8_t i=0; i<3; i++) {
		usart_sendbuffer[2*i] = (uint16_t)(512+controls.gyro_data[i])>>8;
		usart_sendbuffer[2*i+1] = (uint16_t)(512+controls.gyro_data[i]);
	}
	usart_sendbuffer[6] = 0xBB;
	USART_async_send_start(7);
}

void handlePanicCommand() {
	copter_state = 99; //panic mode
	usart_sendbuffer[6] = 0x00;
	USART_async_send_start(1);
}


/*------------------------------*/
/*     USART BLOCKING TXRX      */
/*------------------------------*/


char USART_recv_byte() {
	while ((UCSRA & (1 << RXC)) == 0); // Do nothing until data have been recieved and is ready to be read from UDR 
	return UDR;
}


void USART_send_byte(char data) {
	while ((UCSRA & (1 << UDRE)) == 0); // Do nothing until UDR is ready for more data to be written to it 
	UDR = data;
}


void setup_USART() {
	UCSRB |= (1 << RXEN) | (1 << TXEN) | (1 << RXCIE) | (1 << TXCIE);   // Turn on the transmission and reception circuitry 
	UCSRC |= (1 << URSEL) | (1 << UCSZ0) | (1 << UCSZ1); // Use 8-bit character sizes 

	UBRRL = BAUD_PRESCALE; // Load lower 8-bits of the baud rate value into the low byte of the UBRR register 
	UBRRH = (BAUD_PRESCALE >> 8); // Load upper 8-bits of the baud rate value into the high byte of the UBRR register 
}
