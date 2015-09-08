/*
 * \file main.c
 * \brief Provides the reset vector and the main loop
 * \author Michael Spiegel, <michael.h.spiegel@gmail.com>
 *
 * Copyright (C) 2015 Michael Spiegel
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; version 2 of the License
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License along
 * with this program; if not, write to the Free Software Foundation, Inc.,
 * 51 Franklin Street, Fifth Floor, Boston, MA 02110-1301 USA.
 */

#include "am2303.h"

#include <avr/io.h>
#include <util/delay.h>
#include <avr/sfr_defs.h>
#include <avr/interrupt.h>

/** \brief The UART's baud rate */
#define BAUD 9600
#include <util/setbaud.h>

void main_recordData(status_t status, uint16_t temperature, uint16_t humidity,
		uint8_t channel);

int main(void) {

	DDRB |= _BV(PB1);

	// Enable UART output: 8-E-2
	UCSRA = USE_2X << U2X;
	UCSRB = _BV(TXEN);
	UCSRC = _BV(URSEL) | _BV(UPM1) | _BV(USBS) | _BV(UCSZ1) | _BV(UCSZ0);
	UBRRL = UBRRL_VALUE;
	UBRRH = UBRRH_VALUE;

	am2303_init();

	sei();

	while (1) {

		PORTB |= _BV(PB1);

		_delay_ms(2500);

		PORTB &= ~_BV(PB1);

		_delay_ms(2500);

		am2303_startReading(1, &main_recordData);

	}

	return 0;
}

/**
 * \brief Test function which writes the received values to the UART
 */
void main_recordData(status_t status, uint16_t temperature, uint16_t humidity,
		uint8_t channel) {

	loop_until_bit_is_set(UCSRA, UDRE);
	UDR = 0xAA;

	loop_until_bit_is_set(UCSRA, UDRE);
	UDR = (uint8_t) status;

	loop_until_bit_is_set(UCSRA, UDRE);
	UDR = (uint8_t) (temperature >> 8);

	loop_until_bit_is_set(UCSRA, UDRE);
	UDR = (uint8_t) temperature;

	loop_until_bit_is_set(UCSRA, UDRE);
	UDR = (uint8_t) (humidity >> 8);

	loop_until_bit_is_set(UCSRA, UDRE);
	UDR = (uint8_t) humidity;

	loop_until_bit_is_set(UCSRA, UDRE);
	UDR = (uint8_t) channel;

}
