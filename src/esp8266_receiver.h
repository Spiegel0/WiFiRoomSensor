/**
 * \file esp8266_receiver.h
 * \brief Specifies the ESP8266 receiver interface
 * \details The receiver interface consists of a callback specification and some
 * functions which allow access to the buffer of the receiver. They may be used
 * as a reduced and safe to use interface which may be deployed to decode
 * packets. The receiver callback has to be registered via the
 * \ref esp8266_session.h module.
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
#ifndef ESP8266_RECEIVER_H_
#define ESP8266_RECEIVER_H_

#include "error.h"
#include <stdint.h>

/**
 * \brief Callback pointer which points to a message processing function.
 * \details The function is executed if a message was received.
 * \param status The status of the message
 * \param channel The channel number of the message. The value ranges from zero
 * to three.
 * \param size The number of data bytes which were received
 * \param rrbID An index to the first byte of the message in the round robin
 * buffer. It is advised to use protected access mechanisms only. The message
 * may not be organized in a consecutive memory block. I.e. the memory may
 * eventually wrap. The identifier stays valid until the function returns.
 */
typedef void (*esp8266_transc_messageReceived)(status_t status, uint8_t channel,
		uint8_t size, uint8_t rrbID);

/**
 * \brief Accesses a previously received byte
 * \details It is assumed that the function is only executed inside the receive
 * callback function. After the callback function has returned, the buffer may
 * not be valid anymore. It is further assumed that all parameters are in valid
 * ranges.
 * \param rrbID A valid round robbin buffer ID which points to the start of the
 * message. See \ref esp8266_transc_messageReceived for more details.
 * \param offset The offset inside the received message. It is expected that the
 * parameter is always strictly lower than the size of the message.
 */
uint8_t esp8266_receiver_getByte(uint8_t rrbID, uint8_t offset);

#endif /* ESP8266_RECEIVER_H_ */
