/*
 * \file system_timer.c
 * \brief The file implements the global system timer
 * \details The timer works interrupt driven and doesn't need regular
 * maintenance. It uses the following hardware resources:
 * <ul>
 * 	<li>8-bit Timer/Counter2</li>
 * </ul
 * \author Michael Spiegel, <michael.h.spiegel@gmail.com>
 *
 * Copyright (C) 2016 Michael Spiegel
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

#include "system_timer.h"
#include <avr/io.h>
#include <avr/interrupt.h>
#include <util/atomic.h>

#ifndef F_CPU
#warning "The CPU frequency F_CPU is not defined. Assume 8 MHz."
#define F_CPU (8000000UL)
#endif

/** \brief Structure which encapsulates the data of the module */
volatile struct {
	/** \brief A flag which indicates that the timer has been triggered before */
	int8_t fired :1;
	/** \brief An internal counter which further divides the timer frequency */
	uint8_t cnt :6;
} system_timer_data;

void system_timer_init(void) {

	TCCR2 = _BV(CS22) | _BV(CS21) | _BV(CS20);
	ASSR = 0;

	TIFR = _BV(TOV2); // Clear interrupt flag
	TIMSK |= _BV(TOIE2);

	system_timer_data.fired = 0;
	system_timer_data.cnt = 0;
}

/**
 * \brief Increases the internal counter and maintains the fired flag
 */
ISR(TIMER2_OVF_vect, ISR_BLOCK) {

	system_timer_data.cnt++;

	if (system_timer_data.cnt >=
			(F_CPU / 1000UL * SYSTEM_TIMER_PERIOD_MS / 256UL / 1024UL)) {
		system_timer_data.fired = 1;
		system_timer_data.cnt = 0;
	}
}

uint8_t system_timer_query(void) {
	uint8_t ret;
	ATOMIC_BLOCK(ATOMIC_RESTORESTATE){
		ret = system_timer_data.fired;
		system_timer_data.fired = 0;
	}
	return ret;
}
