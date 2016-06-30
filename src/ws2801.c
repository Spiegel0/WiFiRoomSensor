/**
 * \file ws2801.c
 * \brief Implements the LED chain driver
 * \details The module occupies the following hardware resources:
 * <ul>
 * 	<li> SCK (PB5) </li>
 * 	<li> MISO (PB4), may stay unconnected </li>
 * 	<li> MOSI (PB3) </li>
 * 	<li> SS (PB2), may stay unconnected. It will be configures as an input with
 * 	enabled pull ups. Do not drive low. After the ws2801_init function is
 * 	called, the pin may be configured as an output externally. Configuring as an
 * 	input and driving the pin low will result in an error.</li>
 * 	<li> SPI core </li>
 * </ul>
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

#include "ws2801.h"

#include "system_timer.h"

#include <avr/io.h>
#include <avr/interrupt.h>
#include <util/atomic.h>
#include <string.h>

/** \brief The channel number of the red LED */
#define WS2801_RED_CHN (0)
/** \brief The channel number of the green LED */
#define WS2801_GREEN_CHN (2)
/** \brief The channel number of the blue LED */
#define WS2801_BLUE_CHN (1)

/** \brief Defines the global states of the module */
typedef enum {
	IDLE, ///< Waiting for a new request
	WRITE_DATA, ///< Writes the data to the LED chain
	LATCH ///< Waits until the data is latched.
} ws2801_state_t;

/** \brief The current state of the module */
ws2801_state_t ws2801_state;

/**
 * \brief Encodes the progress of the current operation.
 * \details If the module is in the WRITE_DATA state it encodes the currently
 * written byte of the buffer. If the module is in the LATCH state, it holds the
 * number of system timer ticks until the IDLE state may entered.
 */
uint8_t ws2801_progress;

/**
 * \brief The central data buffer which holds the complete image
 * \details The buffer stores the single pixels in groups of three bytes each.
 * The byte order corresponds to the byte order of each chip.
 */
uint8_t ws2801_data_buffer[3 * WS2801_CHAIN_SIZE];

void ws2801_init(void) {

	ws2801_state = IDLE;

	// Initialize PB2 (SS)
	DDRB &= ~_BV(PB2);
	DDRB |= _BV(PB5) | _BV(PB3);
	PORTB |= _BV(PB2);

	// Master, enabled interrupts, MSBit first, sample on rising edge,
	// low when idle
	SPCR = _BV(SPIE) | _BV(SPE) | _BV(MSTR);
	SPSR = _BV(SPI2X);

	memset(ws2801_data_buffer, 0x00, sizeof(ws2801_data_buffer));
}

void ws2801_timedTick(void) {

	ATOMIC_BLOCK(ATOMIC_RESTORESTATE)
	{
		if (ws2801_state == LATCH) {

			if (ws2801_progress == 0) {
				ws2801_state = IDLE;
			} else {
				ws2801_progress--;
			}

		}
	}
}

status_t ws2801_setValue(uint8_t position, uint8_t red, uint8_t green,
		uint8_t blue) {
	ws2801_state_t state;
	uint8_t i;
	ATOMIC_BLOCK(ATOMIC_RESTORESTATE)
	{
		state = ws2801_state;
	}

	if (state == IDLE || state == LATCH) {

		if (position >= WS2801_CHAIN_SIZE) {
			// Set all pixels
			for (i = 0; i < WS2801_CHAIN_SIZE; i++) {
				ws2801_data_buffer[3 * i + WS2801_RED_CHN] = red;
				ws2801_data_buffer[3 * i + WS2801_GREEN_CHN] = green;
				ws2801_data_buffer[3 * i + WS2801_BLUE_CHN] = blue;
			}
		} else {
			// Set the given pixel
			ws2801_data_buffer[3 * position + WS2801_RED_CHN] = red;
			ws2801_data_buffer[3 * position + WS2801_GREEN_CHN] = green;
			ws2801_data_buffer[3 * position + WS2801_BLUE_CHN] = blue;
		}

		return success;
	} else {
		return err_invalidState;
	}
}

status_t ws2801_update(void) {
	ws2801_state_t state;
	ATOMIC_BLOCK(ATOMIC_RESTORESTATE)
	{
		state = ws2801_state;
	}

	if (state == IDLE) {

		ws2801_progress = 0;
		ws2801_state = WRITE_DATA;
		SPDR = ws2801_data_buffer[0];

		return success;
	} else {
		return err_invalidState;
	}
}

/**
 * \brief checks whether the buffer is fully transmit and initiates sending the
 * next byte.
 * \details It is assumed that the ISR is only called in state WRITE_DATA
 */
ISR(SPI_STC_vect, ISR_NOBLOCK) {
	ws2801_progress++;
	if (ws2801_progress
			< sizeof(ws2801_data_buffer) / sizeof(ws2801_data_buffer[0])) {
		SPDR = ws2801_data_buffer[ws2801_progress];
	} else {
		ws2801_state = LATCH;
		ws2801_progress = SYSTEM_TIMER_MS_TO_TICKS(1);
	}
}
