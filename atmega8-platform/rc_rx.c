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
#include "rc_rx.h"
#include "avr/interrupt.h"
#include "defines.h"
#include "util_usart.h"


#define RX_HIGH (PIND & _BV(PD2))	// Input bit is high?
#define RX_FIRSTBIT_LENGTH 72   // 24+48 = 72
#define RX_BIT_LENGTH 48


volatile uint8_t rx_bit_index = 0;
volatile uint8_t rx_byte_index = 0;

volatile uint8_t rx_overrun_bit = 0;

volatile uint16_t rx_income_data[9];
volatile uint16_t rx_verified_data[8];

volatile uint16_t rx_crc_byte = 0;

volatile uint8_t crc_fault = 0;

volatile uint8_t rx_new_data_avail = 0;


void rx_resync() {
	uint8_t ones = 0;	// Start capturing only right time
	do {
		_delay_us(12);
		if (RX_HIGH) { ++ones; }
		else { ones = 0; }
	} while (ones<200);
	GICR |= _BV(INT0);	// Switch on receiving
}


ISR(INT0_vect) {	// New byte to be received
	if (MCUCR != 0)	{	// Rising edve detection
		MCUCR = 0;	// Low level detection
		return;
	}
	OCR2 = RX_FIRSTBIT_LENGTH;   // Start timer0, top=24+48
	TCNT2 = 0;
	SFIOR |= _BV(PSR2);		// Reset prescale
	TCCR2 = _BV(CS21) | _BV(WGM21);  // 8 prescale for Timer2, CTC (Clear on Compare Match) flag
	GICR &= ~_BV(INT0); // Switch INT0_vect off (self)
}



ISR(TIMER2_COMP_vect) {  // Receiving one bit from RX
	// 12/24us passed
	OCR2 = RX_BIT_LENGTH;   // TOP = 48

	if (RX_HIGH) {
		rx_income_data[rx_byte_index] |= (1<<rx_bit_index);
	}

	rx_bit_index++;
	if (rx_bit_index == 16) {
		rx_bit_index = 0;

		TCCR2 = 0;  // Switch off Timer2 (self)

		if (rx_byte_index == 8) {
			rx_byte_index = 0;
			//if ((uint8_t)rx_income_data[8] == (uint8_t)rx_crc_byte) && ((uint8_t)(rx_income_data[8]>>8) == ) {	// CRC OK
			if (rx_income_data[8] == rx_crc_byte) {	// CRC OK
				__FLASH_LED;
				for (uint8_t i=0; i<9; i++) {
					rx_verified_data[i] = rx_income_data[i];
				}
				rx_new_data_avail = 1;
				crc_fault = 0;
			} else {
				if (++crc_fault == 5) {
					crc_fault = 0;
					sei();
					rx_resync();	// Resync
					cli();
				}
			}
			for (uint8_t i=0; i<9; i++) {
				rx_income_data[i] = 0;
			}
			rx_crc_byte = 0;
		} else {
			rx_crc_byte += rx_income_data[rx_byte_index++];
		}

		if (!RX_HIGH)
			MCUCR = _BV(ISC00) | _BV(ISC01);	// Rising edge generates interrupt
		GICR |= _BV(INT0);
	}
}



void setup_RX() {
	MCUCR = 0;	// Low generates interrupt
	TIMSK |= _BV(OCIE2);	// Timer2 OVF interrupt enable

	for (uint8_t i=0; i<9; i++) {	// Reset input values
		rx_income_data[i] = 0;
	}
	for (uint8_t i=0; i<8; i++) {
		rx_verified_data[i] = 0;
	}

	rx_resync();
}

