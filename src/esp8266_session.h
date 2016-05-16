/*
 * \file esp8266_session.h
 * 
 * \brief The file specifies the interface of the ESP8266 session control module
 * \details <p> The module drives the ESP8266 chip by the AT command set. It
 * configures the properties of the chip and encapsulates outgoing messages.
 * Incoming messages will have to be fetched from the esp8266 transceiver module
 * directly in order to avoid data copy operations or costly wrapper functions.
 * However, the receive callback function still has to be set in this module.
 * </p>
 * <p> The chip will be configured as server which listens on the statically
 * configured port. </p>
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
#ifndef ESP8266_SESSION_H_
#define ESP8266_SESSION_H_

#include "error.h"
#include "esp8266_transceiver.h"

/**
 * \brief Indicates that the send operation was completed
 * \param status The status of the previously initiated send operation.
 */
typedef void (*esp8266_session_sendComplete)(status_t status);

/**
 * \brief Initializes the ESP8266
 * \details <p> The function initializes the transceiver layer and the ESP8266.
 * It uses the configuration header file to determine the network parameters.
 * The chip may not be immediately able to send outgoing packages. First, a
 * client has to connect. Since the ESP8266 will be operated as server, no
 * callback is provided which indicates that the initialization function has
 * finished. The initialization function must be called exactly once before
 * using any other function of the module. </p>
 * <p> Currently, it is assumed that the ESP8266 was properly configured in
 * station mode connecting to a given network. The network parameters will not
 * be set again. </p>
 * \param messageCB The transceiver callback function which indicates a received
 * message. It will be directly passed to \ref esp8266_transc_init.
 */
void esp8266_session_init(esp8266_transc_messageReceived messageCB);

/**
 * \brief Sends the given message
 * \details The destination is determined by the given channel identifier. The
 * channel number corresponds to the number passed in the message callback. The
 * buffer must remain valid until the send complete callback is executed. It is
 * not allowed to send more than one message at once. Hence, the function may
 * not be called until the callback function is executed.
 * \param channel A valid channel identifier. It specified the destination
 * socket and must be between zero and three inclusively.
 * \param buffer A pointer to the data buffer. It has to hold at least size
 * elements and must be valid until sendCompleteCB is executed.
 * \param size The number of bytes to send
 * \param sendCompleteCB The callback function which is executed if the sending
 * operation finishes. If the function \ref esp8266_session_send doesn't return
 * successfully, then the callback won't be executed. It is assumed that the
 * reference always points to a valid location.
 * \return success if the operation is started successfully.
 */
status_t esp8266_session_send(uint8_t channel, uint8_t *buffer, uint8_t size,
		esp8266_session_sendComplete sendCompleteCB);

#endif /* ESP8266_SESSION_H_ */
