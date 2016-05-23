/*
 * \file esp8266_session.c
 * 
 * \brief ESP8266 session management implementation
 * \details The module initializes the ESP 8266 transceiver and assembles the
 * commands which are used to send data. It maintains a simple message buffer
 * which is used to hold the commands.
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

#include "esp8266_session.h"

#include "network-config.h"
#include "esp8266_transceiver.h"
#include "system_timer.h"
#include <avr/pgmspace.h>
#include <stdint.h>
#include <string.h>

/** \brief The length of the internal message buffer in bytes */
#define ESP8266_SESSION_BUFFER_SIZE (128)

/** \brief The command buffer of the module */
static uint8_t esp8266_session_buffer[ESP8266_SESSION_BUFFER_SIZE];

/**
 * \brief Encodes the state of the module.
 * \details If the \ref esp8266_session_statusReceived function is called, the
 * variable gives the current state and the function may change the state to
 * the logically successive state.
 */
static enum {
	IDLE = 0, ///< \brief No operation is performed
	INIT_WAIT, ///< \brief Waits until the chip has initialized itself
	INIT_SETMUX, ///< \brief Sets the multiplexing setting (multiple connections)
	INIT_OPENSRV, ///< \brief Opens the TCP/IP Server
	/**
	 * \brief Waits until the initialization procedure is started again
	 * \details Before starting the initialization procedure the chip is reset.
	 */
	INIT_LONG_RETRY
} esp8266_session_state;

/**
 * \brief The number of remaining timedTicks until the next operation is
 * scheduled
 */
static uint16_t esp8266_session_remainingTicks;

/** \brief The number of retries which are performed before giving up. */
static uint8_t esp8266_session_retryCnt;

// The whole bunch of command strings
/**
 * \brief Command which sets the chip multiplexing mode
 * \details The chip will be able to serve multiple connections
 */
const char esp8266_session_cmdMux[] PROGMEM = "AT+CIPMUX=1";
/** \brief Command which opens a server */
const char esp8266_session_cmdOpenSrv[] PROGMEM = "AT+CIPSERVER=1,"
NW_CONFIG_SRV_PORT;
/** \brief command which resets the chip */
const char esp8266_session_cmdReset[] PROGMEM = "AT+RST";

// Function definition
void esp8266_session_statusReceived(status_t status);
void esp8266_session_sendCommand_P(const char *command_P);
static void esp8266_session_handleInitError(void);

void esp8266_session_init(esp8266_transc_messageReceived messageCB) {
	// Initialize the transceiver
	esp8266_transc_init(esp8266_session_statusReceived, messageCB);

	// Wait until the chip has been initialized
	esp8266_session_remainingTicks = SYSTEM_TIMER_MS_TO_TICKS(1000);
	esp8266_session_state = INIT_WAIT;
	esp8266_session_retryCnt = 3;

}

/**
 * \brief Main entry point of the session state machine
 * \details The function implements the state machine which manages
 * initialization and any sending operations.
 * \param status The status which is returned from the ESP8266
 */
void esp8266_session_statusReceived(status_t status) {

	switch (esp8266_session_state) {
	case INIT_SETMUX: // ---------------------------------------------------------
		if (status == success) {
			esp8266_session_state = INIT_OPENSRV;
			esp8266_session_sendCommand_P(esp8266_session_cmdOpenSrv);
		} else {
			esp8266_session_handleInitError();
		}
		break;

	case INIT_OPENSRV: // --------------------------------------------------------
		if (status == success) {
			esp8266_session_state = IDLE;
		} else {
			esp8266_session_handleInitError();
		}
		break;

	default: // ------------------------------------------------------------------
		break;
	}

}

/**
 * \brief handles an error during initialization of the esp8266
 * \details It checks the retry counter and starts the procedure anew. If no
 * retries are left, the long retry mode is entered.
 */
static void esp8266_session_handleInitError(void) {
	if (esp8266_session_retryCnt > 0) {
		esp8266_session_state = INIT_WAIT;
		esp8266_session_retryCnt--;
		esp8266_session_remainingTicks = SYSTEM_TIMER_MS_TO_TICKS(1500UL);
	} else {
		esp8266_session_state = INIT_LONG_RETRY;
		esp8266_session_retryCnt = 1;
		esp8266_session_remainingTicks = SYSTEM_TIMER_MS_TO_TICKS(180000UL);
	}
	esp8266_session_sendCommand_P(esp8266_session_cmdReset);
}

/**
 * \brief Maintains the wait timer and starts appropriate actions
 * \details The timed part of the state machine is implemented here.
 */
// TODO: Check if ESP8266 has been configured before and set operation mode and
//       Network settings
void esp8266_session_timedTick(void) {

	if (esp8266_session_remainingTicks > 0) {
		esp8266_session_remainingTicks--;
	} else {

		switch (esp8266_session_state) {
		case INIT_WAIT: // ---------------------------------------------------------
		case INIT_LONG_RETRY:
			esp8266_session_state = INIT_SETMUX;
			esp8266_session_sendCommand_P(esp8266_session_cmdMux);
			break;

		default: // ----------------------------------------------------------------
			break;
		}

	}

}

/**
 * \brief Initiates sending of command_P
 * \details It is assumed that the message buffer is currently available. After
 * calling the function the buffer will be locked and used by the transceiver.
 * The message buffer is free again if the status code is returned. The function
 * will not check any state nor if the transceiver is ready.
 * \param command_P The zero terminated string which resides in the program
 * memory space. It is assumed that the string fits into the buffer. Since the
 * command is statically allocated, its length will no be checked. It is assumed
 * that the command doesn't include the terminating charge return and newline
 * characters.
 */
void esp8266_session_sendCommand_P(const char *command_P) {
	uint8_t length;
	(void) strcpy_P((char*) esp8266_session_buffer, command_P);
	length = strlen((char*) esp8266_session_buffer);
	esp8266_session_buffer[length++] = '\r';
	esp8266_session_buffer[length++] = '\n';
	esp8266_transc_send(esp8266_session_buffer, length);
}
