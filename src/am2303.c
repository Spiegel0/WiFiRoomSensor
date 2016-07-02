/**
 * \file am2303.c
 * \brief Implements the module which drives the DHT22/AM2303 sensor
 * \details The decoding logic is implemented in an interrupt driven way. It
 * doesn't require a periodic update function. The module occupies the following
 * hardware resources:
 * <ul>
 *   <li>PD2 (INT0): Channel 0, data line </li>
 *   <li>PD3 (INT1): Channel 1, data line </li>
 *   <li>Timer/Counter 0</li>
 * </ul>
 * It is assumed that the hardware resources are not shared.
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

#include "am2303.h"
#include "debug.h"

#include <avr/io.h>
#include <avr/interrupt.h>
#include <stdint.h>

/** \brief IDLE or issue start sequence state */
#define STATE_IDLE (0)
/** \brief Waits until the sensor issues its start sequence */
#define STATE_START (1)
/** \brief Intermediate state which waits for the first bit's falling edge */
#define STATE_BEGIN_TRANSMISSION (2)
/** \brief waits until the next bit can be read */
#define STATE_READ_WAIT (3)
/** \brief Measures the time in order to determine the bit value */
#define STATE_READ_MEASURE (4)
/** \brief Waits for the last rising edge and processes the message */
#define STATE_AWAIT_LAST_EDGE (5)

/** \brief The number of bytes per message including the checksum byte */
#define AM2303_MESSAGE_SIZE (5)

/**
 * \brief encapsulates the module's data
 * \details After am2303_startReading() was invoked the structure's members are
 * only accessed by interrupt routines. Any access while the module is in an
 * active state may corrupt the data structure.
 */
volatile struct {
	uint8_t state :3; //< \brief The module's current state
	uint8_t byteNr :3; //< \brief The currently received byte
	uint8_t bitNr :3; //< \brief The currently received bit number
	uint8_t channel :1; //< \brief The requested channel number
} am2303_data;

/** \brief The assigned callback function */
am2303_readDone_t am2303_callback;

/** \brief A buffer which contains the received message */
uint8_t am2303_messageBuffer[AM2303_MESSAGE_SIZE];

/**
 * \brief returns the number of timer ticks for the given configuration
 * \param prescaler The currently used prescaler
 * \param us2top the time consumed by all ticks
 */
#define AM2303_TICK_VAL(prescaler,us2top) \
	((F_CPU/(prescaler)*(us2top) + 1000000UL - 1)/(1000000UL))

/* Function prototypes */
inline void am2303_processMessage(void);

void am2303_init(void) {
	// Initialize output pins
	DDRD &= ~(_BV(PD2) | _BV(PD2));
	PORTD |= _BV(PD2) | _BV(PD2);

	// Disarm timer
	TIMSK &= ~(_BV(TOIE0));

	// Disarm external interrupts
	GICR &= ~(_BV(INT0) | _BV(INT1));

	// Initialize state
	am2303_data.state = STATE_IDLE;
}

void am2303_startReading(uint8_t channel, am2303_readDone_t callback) {

	am2303_callback = callback;
	am2303_data.channel = channel & 0x01;
	am2303_data.state = STATE_IDLE;
	am2303_data.byteNr = 0;
	am2303_data.bitNr = 0;

	switch (channel) {
	case 0:
		DDRD |= _BV(PD2);
		PORTD &= ~_BV(PD2);
		break;
	case 1:
		DDRD |= _BV(PD3);
		PORTD &= ~_BV(PD3);
		break;
	default:
		callback(err_invalidChannel, 0, 0, channel);
	}

	// Setup timer to fire in ~18ms
	TCCR0 = _BV(CS02) | _BV(CS00); // Prescaler 1024
	TIFR |= _BV(TOV0); // Clear flag
	TCNT0 = 256 - AM2303_TICK_VAL(1024UL,18000UL);

	TIMSK |= _BV(TOIE0);
}

/**
 * \brief Performs a timed action
 * \details If the state of the module is IDLE, the data line will be released
 * and further actions will be prepared. Otherwise an error will be reported by
 * calling the callback function.
 */
ISR(TIMER0_OVF_vect, ISR_BLOCK) {

	if (am2303_data.state == STATE_IDLE) {

		am2303_data.state = STATE_START;

		// Release data line
		DDRD &= ~(_BV(PD2) | _BV(PD3));
		PORTD |= _BV(PD2) | _BV(PD3);

		// Set interrupt to next rising edge
		MCUCR |= _BV(ISC11) | _BV(ISC10) | _BV(ISC01) | _BV(ISC00);
		GIFR |= _BV(INTF1) | _BV(INTF0);
		switch (am2303_data.channel) {
		case 0:
			GICR |= _BV(INT0);
			break;
		case 1:
			GICR |= _BV(INT1);
			break;
		}

		// Set timeout >120us
		TCCR0 = _BV(CS01); // Prescaler 8
		TCNT0 = 136;
		TIFR |= _BV(TOV0); // Clear interrupt flag

	} else {
		// Stop the timer and issue an error
		TIMSK &= ~(_BV(TOIE0));
		GICR &= ~(_BV(INT0) | _BV(INT1));
		am2303_callback(err_noSignal, am2303_data.state, 0, am2303_data.channel);
		am2303_data.state = STATE_IDLE;
	}
}

/**
 * \brief Decodes the message after the initial start sequence was received
 * \details The ISR may be used for INT0 and INT1. It runs the state machine and
 * follows the received bit stream. It uses timer 0 in order to measure the
 * interval between two edges. During normal operation, timer interrupts will
 * not be executed.
 */
ISR(INT0_vect, ISR_BLOCK) {
	uint8_t cnt = TCNT0;
	TCNT0 = 0;

	if (am2303_data.state == STATE_START) {
		// rising edge of the start bit
		// -> wait until the next falling edge (first bit)
		am2303_data.state = STATE_BEGIN_TRANSMISSION;

		// Trigger an interrupt on any state change
		switch (am2303_data.channel) {
		case 0:
			MCUCR |= _BV(ISC00);
			MCUCR &= ~_BV(ISC01);
			break;
		case 1:
			MCUCR |= _BV(ISC10);
			MCUCR &= ~_BV(ISC11);
			break;
		}

	} else if (am2303_data.state == STATE_BEGIN_TRANSMISSION) {
		am2303_data.state = STATE_READ_WAIT;
	} else if (am2303_data.state == STATE_READ_WAIT) {
		am2303_data.state = STATE_READ_MEASURE;
	} else if (am2303_data.state == STATE_READ_MEASURE) {
		// Process bit
		uint8_t bit = (cnt > 49 ? 1 : 0);
		am2303_messageBuffer[am2303_data.byteNr] <<= 1;
		am2303_messageBuffer[am2303_data.byteNr] |= bit;

		am2303_data.state = STATE_READ_WAIT;

		if (am2303_data.bitNr == 7) {
			// Byte is fully populated
			am2303_data.bitNr = 0;
			if (am2303_data.byteNr == AM2303_MESSAGE_SIZE - 1) {
				// Message is fully received
				am2303_data.byteNr = 0;
				am2303_data.state = STATE_AWAIT_LAST_EDGE;

			} else {
				am2303_data.byteNr++;
			}
		} else {
			am2303_data.bitNr++;
		}
	} else if (am2303_data.state == STATE_AWAIT_LAST_EDGE) {
		am2303_data.state = STATE_IDLE;

		// disarm the timer and external interrupts
		TIMSK &= ~(_BV(TOIE0));
		GICR &= ~(_BV(INT0) | _BV(INT1));

		// Process message
		am2303_processMessage();
	}
}

/*** \brief Redirects INT1 to INT0 which handles both interrupts */
ISR(INT1_vect, ISR_ALIASOF(INT0_vect));

/**
 * \brief Processes the previously received message
 * \details The function assumes that interrupts are turned off. It may
 * temporarily enable interrupts during non critical sections. After the message
 * is decoded, the callback function is invoked. The function requires a fully
 * populated message buffer and may be called in an interrupt context
 */
inline void am2303_processMessage(void) {
	uint8_t chksum;
	uint16_t temperature, humidity;

	sei();

	chksum = am2303_messageBuffer[0];
	chksum += am2303_messageBuffer[1];
	chksum += am2303_messageBuffer[2];
	chksum += am2303_messageBuffer[3];

	humidity = am2303_messageBuffer[1] | (am2303_messageBuffer[0] << 8);
	temperature = am2303_messageBuffer[3] | (am2303_messageBuffer[2] << 8);

	am2303_callback((chksum == am2303_messageBuffer[4] ? success : err_chksum),
			temperature, humidity, am2303_data.channel);

	cli();
}
