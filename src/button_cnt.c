/**
 * \file button_cnt.c
 * \brief Implements the button counter module
 * \details Used Hardware:
 * <ul>
 * 	<li>PC0 (OK)</li>
 * 	<li>PC1 (Up)</li>
 * 	<li>PC2 (Down)</li>
 * </ul>
 * A simple debouncing algorithm is used for all buttons. For each button a
 * shift register is used which counts the number of consecutive pressed
 * samples.
 *
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

#include "button_cnt.h"

#include <avr/io.h>
#include <stdint.h>

#include "debug.h"

/** \brief The number of queried buttons. */
#define BUTTON_CNT_CHANNELS (3)

/** \brief The port register of the buttons to sample */
#define BUTTON_CNT_PORT (PORTC)
/** \brief The data direction register of the buttons to sample */
#define BUTTON_CNT_DDR (DDRC)
/** \brief The pin register of the buttons to sample */
#define BUTTON_CNT_PIN (PINC)

/**
 * \brief The bit number of the first IO pin
 * \details The other sampled pins follow consecutively
 */
#define BUTTON_CNT_FIRST_BIT (PINC0)

/**
 * \brief The number of locked samples until the value is taken
 */
#define BUTTON_CNT_WAIT_SAMPLES (3)

/** \brief The sampled state for each button */
static uint8_t button_cnt_states[BUTTON_CNT_CHANNELS];

/** \brief The current value of the button counter */
static int16_t button_cnt_value;

/** \brief The callback function to propagate changes */
static button_cnt_callback_t button_cnt_callback;

void button_cnt_init(button_cnt_callback_t callback) {
	uint8_t i;

	for (i = 0; i < BUTTON_CNT_CHANNELS; i++) {
		BUTTON_CNT_DDR &= ~_BV(BUTTON_CNT_FIRST_BIT + i);
		BUTTON_CNT_PORT |= _BV(BUTTON_CNT_FIRST_BIT + i);
		button_cnt_states[i] = 0;
	}
	button_cnt_value = 0;
	button_cnt_callback = callback;
}

void button_cnt_timedFastTick(void) {
	uint8_t i;
	uint8_t pressed = 0;
	uint8_t maskedValue;

	for (i = 0; i < BUTTON_CNT_CHANNELS; i++) {
		// Shift the first bit into the state
		button_cnt_states[i] <<= 1;
		button_cnt_states[i] |= (BUTTON_CNT_PIN >> (BUTTON_CNT_FIRST_BIT + i))
				& 0x01;
		// Mask the first WAIT_SAMPLES + 1 bits
		maskedValue = button_cnt_states[i]
				& ((1 << (BUTTON_CNT_WAIT_SAMPLES + 1)) - 1);
		// Check if the masked value equals 10...0
		pressed |= (maskedValue == (1 << BUTTON_CNT_WAIT_SAMPLES)) << i;
	}

	if (pressed & 0x02) // Up
		button_cnt_value++;
	if (pressed & 0x04) // Down
		button_cnt_value--;

	if (pressed) {
		button_cnt_callback(button_cnt_value, pressed);
	}
}
