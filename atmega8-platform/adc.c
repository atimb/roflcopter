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

#include "adc.h"
#include "defines.h"
#include "avr/interrupt.h"


ISR(ADC_vect) {
	adc_gyro_data[ADMUX] = ((adc_gyro_data[ADMUX]*3)+ADC)>>2;
	if (ADMUX < 2) {
		++ADMUX;
		ADCSRA |= _BV(ADSC);	// Start new conversion
	} else {
		ADMUX = 0;
	}
}


ISR(TIMER0_OVF_vect) {  // Reading the gyro rates, approx ~61Hz

	ADMUX = 0;
	ADCSRA |= _BV(ADSC);	// Start new conversion

}


void setup_ADC() {
	for (int i=0; i<3; ++i) {
		adc_gyro_data[i] = 0;
	}
	TIMSK |= _BV(TOIE0);	// Timer0 OVF interrupt enable
	TCCR0 = _BV(CS00) | _BV(CS02);	// 1024 prescale
	ADCSRA = _BV(ADIE) | _BV(ADPS0) | _BV(ADPS1) | _BV(ADPS2) | _BV(ADEN);
}

