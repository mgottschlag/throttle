/*
Copyright (C) 2012 Mathias Gottschlag <mgottschlag@gmail.com>

This program is free software: you can redistribute it and/or modify
it under the terms of the GNU General Public License as published by
the Free Software Foundation, either version 3 of the License, or
(at your option) any later version.

This program is distributed in the hope that it will be useful,
but WITHOUT ANY WARRANTY; without even the implied warranty of
MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
GNU General Public License for more details.

You should have received a copy of the GNU General Public License
along with this program.  If not, see <http://www.gnu.org/licenses/>.
*/

#include "adc.h"

#include <avr/io.h>

#define ADC_ENABLE 0x80
#define ADC_PRESCALE_DIV128 0x07

#define ADC_REFERENCE_AVCC 0x40

#define ADC_MUX_MASK 0x1F

void adcInit(void) {
	// Enable ADC and set the prescaler
	ADCSRA = ADC_ENABLE | ADC_PRESCALE_DIV128;
	// Set default reference
	ADMUX = ADC_REFERENCE_AVCC;
}
void adcUninit(void) {
	// Disable ADC (turn off ADC power)
	ADCSRA &= ~_BV(ADEN);
}

unsigned short adcConvertChannel(unsigned char channel) {
	// Set channel
	ADMUX = (ADMUX & ~ADC_MUX_MASK) | (channel & ADC_MUX_MASK);
	// Start conversion
	ADCSRA |= _BV(ADSC);
	// wait until conversion complete
	while (bit_is_set(ADCSRA, ADSC));

	// Read ADC (full 10 bits)
	return ADC;
}

