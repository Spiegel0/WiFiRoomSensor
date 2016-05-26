/*
 * \file esp8266_transceiver.c
 * \brief The file implements the UART driver and implements simple decoding
 * tasks
 * \details The module demultiplexes the received input stream and decodes the
 * basic message content and basic status notifications. It deploys a message
 * buffer and out-sources the decoding logic into the main execution loop.
 * Hence, it requires a frequent execution of a tick function. The module
 * requires sole access to the following resources:
 * <ul>
 *   <li>USART</li>
 *   <li>PDO (RxD)</li>
 *   <li>PD1 (TxD)</i>
 * </ul>
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

#include "esp8266_transceiver.h"

#include <avr/io.h>
#include <avr/interrupt.h>
#include <avr/pgmspace.h>

/** \brief If the variable is defined, debug messages are suppressed */
#define ESP8266_TRANSC_NDEBUG
/**
 * \brief The size of the round robin buffer used to store received values
 * \details The size always has to be a power of two.
 */
#define ESP8266_TRANSC_RBUFFER_SIZE (128)

/** \brief The currently sent packet */
static uint8_t *esp8266_transc_sendBuffer;
/** \brief The size of the esp8266_transc_sendBuffer */
static uint8_t esp8266_transc_sendBufferSize;
/** \brief The index of the next byte to send */
static uint8_t esp8266_transc_sendIndex;
/**
 * \brief The index of the next echoed byte inside the send buffer
 * \details If the value is greater or equal than the
 * esp8266_transc_sendBufferSize value, than no data transmission is in
 * progress and no echo is to be expected.
 */
static uint8_t esp8266_transc_nextEcho;

/** \brief Status notification callback function */
static esp8266_transc_statusReceived esp8266_transc_statusCB;
/** \brief Message notification callback function */
static esp8266_transc_messageReceived esp8266_transc_messageCB;

/**
 * \brief The round robin buffer used to store received values
 * \details The buffer may also be accessed in an interrupt context. The
 * accessed regions are determined by the index pointers and will not overlap.
 */
static volatile uint8_t esp8266_transc_rrBuffer[ESP8266_TRANSC_RBUFFER_SIZE];
/**
 * \brief The index of the first valid byte
 * \details The variable may be read in an interrupt context. It is increased if
 * the value was successfully processed.
 */
static uint8_t esp8266_transc_rrFirst;
/**
 * \brief The index of the first character in the round robin buffer which
 * hasn't been processed before.
 * \details The index is used to separate the processed region from the still
 * allocated region. It mustn't be read in an interrupt context. Hence,
 * synchronization is not necessary.
 */
static uint8_t esp8266_transc_rrFirstUnprocessed;
/**
 * \brief The size of the valid block inside the round robin buffer.
 * \details The variable may be read and altered in an interrupt context. It
 * must be modified if an element is successfully processed and
 * esp8266_transc_rrFirst is increased.
 */
static volatile uint8_t esp8266_transc_rrAllocation;

/**
 * \brief A temporary variable which holds the parsed channel ID
 */
static uint8_t esp8266_transc_rcvChannelID;
/**
 * \brief A temporary variable which holds the number of bytes to receive
 */
static uint16_t esp8266_transc_rcvSize;

/**
 * \brief The status of the receiver
 */
static enum {
	IDLE = 0, ///< \brief Waits for the next character
	ERR, ///< \brief Ignores the next chunk until \n is received
	/**
	 * \brief Waits until a status or command is received
	 * \details Every '\r' and '\n' character will be absorbed until a '+' sign or
	 * the first character of the status message is received.
	 */
	NL,
	STATUS_MSG, ///< \brief Read the status message
	BGN_MSG, ///< \brief A + indicates an ESP8266 message
	READ_CHN, ///< \brief Reads the channel number
	READ_LENGTH, ///< \brief Reads the message length
	DATA_IN, ///< \brief Reads the message's data
	READ_NL, ///< \brief Ignores any intermediate '\n' or '\r' characters
	READ_STATUS, ///< \brief The message's status is read
	CMD_PROMPT, ///< \brief A command prompt was transmitted
} esp8266_transc_state;

/** \brief the status ok string */
const char esp8266_transc_str_ok[] PROGMEM = "OK";
/** \brief the status send ok string */
const char esp8266_transc_str_sendOk[] PROGMEM = "SEND OK";
/** \brief status code which indicates no change */
const char esp8266_transc_str_noChange[] PROGMEM = "no change";
/** \brief the received packet message identifier */
const char esp8266_transc_str_rcv[] PROGMEM = "IPD";

/** \brief Adds the two operands modulo the buffer size */
#define ESP8266_TRANSC_RRADD(a, b) (((a) + (b)) % \
		(unsigned) (ESP8266_TRANSC_RBUFFER_SIZE))
/** \brief Subtracts the two operands modulo the buffer size */
// The ESP8266_TRANSC_RBUFFER_SIZE literal needs to be unsigned. Otherwise the
// mod operation will return an invalid (negative) result.
#define ESP8266_TRANSC_RRSUB(a, b) (((a) - (b)) % \
		(unsigned) (ESP8266_TRANSC_RBUFFER_SIZE))

/* Function Declarations */
static inline void esp8266_transc_processNextChar(void);
static void esp8266_transc_decreaseBufferSync(void);
static uint16_t esp8266_transc_rrStringToNumber(uint8_t rrStart, uint8_t rrEnd);
static int8_t esp8266_transc_rrstrcmp_PF(uint8_t rrStart, uint8_t rrEnd,
		const char *ref);

void esp8266_transc_init(esp8266_transc_statusReceived statusCB,
		esp8266_transc_messageReceived messageCB) {

	// Initialize variables
	esp8266_transc_statusCB = statusCB;
	esp8266_transc_messageCB = messageCB;
	esp8266_transc_sendBufferSize = 0;
	esp8266_transc_nextEcho = 0;
	esp8266_transc_rrFirst = 0;
	esp8266_transc_rrFirstUnprocessed = 0;
	esp8266_transc_rrAllocation = 0;
	esp8266_transc_state = IDLE;

	// Initializes the UART to 115200-8-N-1, receive interrupt enabled
#define BAUD (115200)
#include <util/setbaud.h>

	UCSRA = USE_2X << U2X;
	UCSRB = _BV(RXCIE) | _BV(RXEN) | _BV(TXEN);
	UCSRC = _BV(URSEL) | _BV(UCSZ1) | _BV(UCSZ0);
	UBRRL = UBRRL_VALUE;
	UBRRH = UBRRH_VALUE;
}

#ifndef ESP8266_TRANSC_NDEBUG
/**
 * \brief Writes the current state to the UART
 * \details The function uses busy wait until the data is written
 */
static void esp8266_transc_debugState(void) {
	uint8_t uart_state;

	cli();
	uart_state = UCSRB;
	UCSRB &= ~(_BV(TXCIE) | _BV(UDRIE)); // Disable interrupts
	sei();

	while (!(UCSRA & _BV(UDRE)))
	;// Wait
	UDR = 0xAA;// magic number
	while (!(UCSRA & _BV(UDRE)))
	;// Wait
	UDR = (uint8_t) esp8266_transc_state;
	while (!(UCSRA & _BV(UDRE)))
	;// Wait
	UDR = (uint8_t) esp8266_transc_rrAllocation;
	while (!(UCSRA & _BV(UDRE)))
	;// Wait
	UDR = (uint8_t) esp8266_transc_rrFirst;
	while (!(UCSRA & _BV(UDRE)))
	;// Wait
	UDR = (uint8_t) esp8266_transc_rrFirstUnprocessed;
	while (!(UCSRA & _BV(UDRE)))
	;// Wait
	UDR = (uint8_t) esp8266_transc_nextEcho;
	while (!(UCSRA & _BV(UDRE)))
	;// Wait
	UDR = (uint8_t) esp8266_transc_rrBuffer[esp8266_transc_rrFirstUnprocessed];
	while (!(UCSRA & _BV(UDRE)))
	;// Wait
	UDR = (uint8_t) esp8266_transc_rcvChannelID;
	while (!(UCSRA & _BV(UDRE)))
	;// Wait
	UDR = (uint8_t) esp8266_transc_rcvSize;
	while (!(UCSRA & _BV(UDRE)))
	;// Wait
	UDR = 0x55;// magic number
	while (!(UCSRA & _BV(UDRE)))
	;// Wait

	cli();
	UCSRB = uart_state;// Reset the UART state
	sei();
}
#endif

void esp8266_transc_tick(void) {

	cli();
	while (ESP8266_TRANSC_RRSUB(esp8266_transc_rrFirstUnprocessed,
			esp8266_transc_rrFirst) < esp8266_transc_rrAllocation) {

		sei();
		esp8266_transc_processNextChar();
		cli();
	}
	sei();

}

/**
 * \brief Reads the next character and processes it
 * \details The function assumes that at least one character is still
 * unprocessed. I.e. <code>(esp8266_transc_rrFirstUnprocessed -
 * esp8266_transc_rrFirst) % ESP8266_TRANSC_RBUFFER_SIZE <
 * esp8266_transc_rrAllocation</code>. It implements the main state machine of
 * the module.
 */
static inline void esp8266_transc_processNextChar(void) {

	uint8_t cChar = esp8266_transc_rrBuffer[esp8266_transc_rrFirstUnprocessed];

#ifndef ESP8266_TRANSC_NDEBUG
	esp8266_transc_debugState(); // debug the state of the module
#endif

	switch (esp8266_transc_state) {
	case IDLE: 	// ---------------------------------------------------------------
		if (cChar == '\r' || cChar == '\n') {
			esp8266_transc_state = NL;
		} else {
			esp8266_transc_state = ERR;
		}
		esp8266_transc_decreaseBufferSync();
		break;

	case ERR: // -----------------------------------------------------------------
		if (cChar == '\n') {
			esp8266_transc_state = IDLE;
		}
		esp8266_transc_decreaseBufferSync();
		break;

	case NL: // ------------------------------------------------------------------
		if (cChar == '\n' || cChar == '\r') {
			// Consume all '\r' and '\n'
			esp8266_transc_decreaseBufferSync();
		} else if (cChar == '+') {
			esp8266_transc_state = BGN_MSG;
			esp8266_transc_decreaseBufferSync();
		} else if (cChar == '>') {
			esp8266_transc_state = CMD_PROMPT;
			esp8266_transc_decreaseBufferSync();
		} else {
			// Keep the first byte of the status message
			esp8266_transc_state = STATUS_MSG;
		}
		break;

	case STATUS_MSG: 	// ---------------------------------------------------------
		if (cChar == '\r') {
			status_t status = err_status;
			// Evaluate status message
			if (esp8266_transc_rrstrcmp_PF(esp8266_transc_rrFirst,
					esp8266_transc_rrFirstUnprocessed, esp8266_transc_str_ok) == 0) {
				status = success;
			} else if (esp8266_transc_rrstrcmp_PF(esp8266_transc_rrFirst,
					esp8266_transc_rrFirstUnprocessed, esp8266_transc_str_sendOk) == 0) {
				status = success;
			} else if (esp8266_transc_rrstrcmp_PF(esp8266_transc_rrFirst,
					esp8266_transc_rrFirstUnprocessed, esp8266_transc_str_noChange)
					== 0) {
				status = err_noChange;
			}

			// Notify via callback
			esp8266_transc_statusCB(status);

			esp8266_transc_state = ERR; // consume last \n
			esp8266_transc_decreaseBufferSync();
		} else {
			// Do not free the Buffer. It still holds the status
			esp8266_transc_rrFirstUnprocessed = ESP8266_TRANSC_RRADD(
					esp8266_transc_rrFirstUnprocessed, 1);
		}
		break;

	case BGN_MSG: // -------------------------------------------------------------
		if (cChar == ',') {
			// check the message code
			if (esp8266_transc_rrstrcmp_PF(esp8266_transc_rrFirst,
					esp8266_transc_rrFirstUnprocessed, esp8266_transc_str_rcv) == 0) {
				esp8266_transc_state = READ_CHN;
			} else {
				esp8266_transc_state = ERR;
			}
			esp8266_transc_decreaseBufferSync();
		} else if (cChar == ':') {
			esp8266_transc_state = ERR;
			esp8266_transc_decreaseBufferSync();
		} else {
			// Do not free the Buffer. It still holds the message code
			esp8266_transc_rrFirstUnprocessed = ESP8266_TRANSC_RRADD(
					esp8266_transc_rrFirstUnprocessed, 1);
		}
		break;

	case READ_CHN: // ------------------------------------------------------------
		if (cChar == ',') {
			// Convert channel number
			esp8266_transc_rcvChannelID = (uint8_t) esp8266_transc_rrStringToNumber(
					esp8266_transc_rrFirst, esp8266_transc_rrFirstUnprocessed);
			if (esp8266_transc_rcvChannelID >= 4) {
				esp8266_transc_state = ERR;
			} else {
				esp8266_transc_state = READ_LENGTH;
			}
			esp8266_transc_decreaseBufferSync();
		} else if (cChar < '0' || cChar > '9') {
			esp8266_transc_state = ERR;
			esp8266_transc_decreaseBufferSync();
		} else {
			// Do not free the Buffer. It still holds the channel id
			esp8266_transc_rrFirstUnprocessed = ESP8266_TRANSC_RRADD(
					esp8266_transc_rrFirstUnprocessed, 1);
		}
		break;

	case READ_LENGTH: // ---------------------------------------------------------
		if (cChar == ':') {
			esp8266_transc_rcvSize = esp8266_transc_rrStringToNumber(
					esp8266_transc_rrFirst, esp8266_transc_rrFirstUnprocessed);
			if (esp8266_transc_rcvSize >= ESP8266_TRANSC_RBUFFER_SIZE - 10) {
				esp8266_transc_state = ERR;
			} else {
				esp8266_transc_state = DATA_IN;
			}
			esp8266_transc_decreaseBufferSync();
		} else if (cChar < '0' || cChar > '9') {
			esp8266_transc_state = ERR;
			esp8266_transc_decreaseBufferSync();
		} else {
			// Do not free the Buffer. It still holds the length
			esp8266_transc_rrFirstUnprocessed = ESP8266_TRANSC_RRADD(
					esp8266_transc_rrFirstUnprocessed, 1);
		}
		break;

	case DATA_IN: // -------------------------------------------------------------
		if (ESP8266_TRANSC_RRSUB(esp8266_transc_rrFirstUnprocessed,
				esp8266_transc_rrFirst) >= esp8266_transc_rcvSize) {
			// Character after the data sequence
			if (cChar == '\r') {
				esp8266_transc_state = READ_NL;
			} else {
				esp8266_transc_state = ERR;
			}
		}
		esp8266_transc_rrFirstUnprocessed = ESP8266_TRANSC_RRADD(
				esp8266_transc_rrFirstUnprocessed, 1);
		break;

	case READ_NL: // -------------------------------------------------------------
		// Ignore additional \r and \n characters
		if (cChar == '\r' || cChar == '\n') {
			// Process the next character
			esp8266_transc_rrFirstUnprocessed = ESP8266_TRANSC_RRADD(
					esp8266_transc_rrFirstUnprocessed, 1);
		} else {
			esp8266_transc_state = READ_STATUS;
			// Do not consume the current character
		}
		break;

	case READ_STATUS: // ---------------------------------------------------------
		if (cChar == '\r') {
			status_t status = err_status;
			// Evaluate status message
			if (esp8266_transc_rrstrcmp_PF(
					ESP8266_TRANSC_RRSUB(esp8266_transc_rrFirstUnprocessed, 2),
					esp8266_transc_rrFirstUnprocessed, esp8266_transc_str_ok) == 0) {
				status = success;
			}

			esp8266_transc_messageCB(status, esp8266_transc_rcvChannelID,
					esp8266_transc_rcvSize, esp8266_transc_rrFirst);

			esp8266_transc_state = ERR; // Consumes the last '\n'
			esp8266_transc_decreaseBufferSync();
		} else {
			esp8266_transc_rrFirstUnprocessed = ESP8266_TRANSC_RRADD(
					esp8266_transc_rrFirstUnprocessed, 1);
		}
		break;

	case CMD_PROMPT: // ----------------------------------------------------------
		if (cChar == ' ') {
			esp8266_transc_statusCB(err_inputExpected);
			esp8266_transc_state = IDLE;
		} else {
			esp8266_transc_state = ERR;
		}
		esp8266_transc_decreaseBufferSync();
		break;
	}
}

uint8_t esp8266_receiver_getByte(uint8_t rrbID, uint8_t offset) {
	return esp8266_transc_rrBuffer[ESP8266_TRANSC_RRADD(rrbID, offset)];
}

/**
 * \brief Removes one byte from the round-robin buffer
 * \details After advancing the esp8266_transc_rrFirstUnprocessed variable, the
 * buffer content before that variable is freed.
 */
static void esp8266_transc_decreaseBufferSync(void) {
	esp8266_transc_rrFirstUnprocessed = ESP8266_TRANSC_RRADD(
			esp8266_transc_rrFirstUnprocessed, 1);
	cli();
	esp8266_transc_rrAllocation -= ESP8266_TRANSC_RRSUB(
			esp8266_transc_rrFirstUnprocessed, esp8266_transc_rrFirst);
	esp8266_transc_rrFirst = esp8266_transc_rrFirstUnprocessed;
	sei();
}

/**
 * \brief Converts the decimal string inside the rr-buffer into a number
 * \details It is expected that the string only contains valid character. The
 * buffer's content has to be checked before. The function does not handle
 * negative numbers or exponential notations. Furthermore, it is assumed that
 * all parameters are valid indices.
 * \param rrStart The index of the first character inside the round robin
 * buffer.
 * \param rrEnd The first index in the rrBuffer after the decimal number.
 * \return The converted number. If the string is empty, zero is returned.
 */
static uint16_t esp8266_transc_rrStringToNumber(uint8_t rrStart, uint8_t rrEnd) {
	uint16_t ret = 0;
	while (rrStart != rrEnd) {
		ret *= 10;
		ret += (uint8_t) (esp8266_transc_rrBuffer[rrStart] - '0');
		rrStart = ESP8266_TRANSC_RRADD(rrStart, 1);
	}
	return ret;
}

/**
 * \brief Compares the string with the content of the round robin buffer.
 * \param rrStart The index of the first byte which has to be compared. It is
 * assumed that the index is smaller than the buffer size. The content of the
 * round robin buffer may not be null terminated. Hence, the string ends with
 * the character at rrEnd. It has to be at least one character in size.
 * \param rrEnd The index after the last character of the string inside the
 * round robin buffer. The character at the given position is excluded.
 * \param ref The zero terminated reference string inside the program memory
 * \return Returns a positive value if the buffered value is greater than the
 * the reference string. Zero is returned if both strings are equal and a
 * negative value is returned otherwise.
 */
// FIXME: Needs to return uint16_t
static int8_t esp8266_transc_rrstrcmp_PF(uint8_t rrStart, uint8_t rrEnd,
		const char *ref) {

	do {

		if (esp8266_transc_rrBuffer[rrStart] != pgm_read_byte(ref)) {
			return esp8266_transc_rrBuffer[rrStart] - pgm_read_byte(ref);
		}

		rrStart = ESP8266_TRANSC_RRADD(rrStart, 1);
		ref++;
	} while ((rrStart != rrEnd) & (pgm_read_byte(ref) != '\0'));

	// Check the length of both strings
	if (rrStart == rrEnd) {
		if (pgm_read_byte(ref) == '\0') {
			return 0;
		} else {
			return -1;
		}
	} else {
		return 1;
	}

}

/**
 * \brief Processes the newly received byte
 * \details Checks whether the currently received byte is an echoed one. If
 * not, it will be added to the rrBuffer. If the rrBuffer is already fully
 * allocated, the byte will be dropped.
 */
ISR(USART_RXC_vect, ISR_BLOCK) {
	uint8_t rcv = UDR;
	uint8_t storeByte = 1;

	// Byte is read and the interrupt source is disarmed

	if (esp8266_transc_nextEcho < esp8266_transc_sendBufferSize) {
		// Check echo
		if (rcv == esp8266_transc_sendBuffer[esp8266_transc_nextEcho]) {
			storeByte = 0;
			esp8266_transc_nextEcho++;
		}
	}

	if (storeByte & (esp8266_transc_rrAllocation < ESP8266_TRANSC_RBUFFER_SIZE)) {
		// Store byte
		esp8266_transc_rrBuffer[ESP8266_TRANSC_RRADD(esp8266_transc_rrFirst,
				esp8266_transc_rrAllocation)] = rcv;
		esp8266_transc_rrAllocation++;
	}
}

void esp8266_transc_send(uint8_t *buffer, uint8_t size) {

	cli();
	esp8266_transc_nextEcho = 0;
	esp8266_transc_sendBufferSize = size;
	esp8266_transc_sendBuffer = buffer;
	sei();
	esp8266_transc_sendIndex = 1;

	UDR = buffer[0];

	// Enable data register empty interrupt
	UCSRB |= _BV(UDRIE);
}

/**
 * \brief Sends the next byte or disables the interrupt
 * \details The ISR will send the next byte, if any. If the buffer is fully
 * transmit, the UDRE interrupt will be disabled.
 */
ISR(USART_UDRE_vect, ISR_BLOCK) {

	if (esp8266_transc_sendIndex < esp8266_transc_sendBufferSize) {
		UDR = esp8266_transc_sendBuffer[esp8266_transc_sendIndex];
		esp8266_transc_sendIndex++;
	} else {
		// Disable interrupt
		UCSRB &= ~_BV(UDRIE);
	}
}
