/**
 * \file debug.h
 * \brief The file contains some debugging aids
 * \details Debugging can be turned off by defining the variable NDEBUG. The
 * module uses the soft uart implementation and a binary debugging format. The
 * first byte is a magic number followed by the debugging information. Before
 * using any debugging directive, the DEBUG_INIT macro has to be instantiated.
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
#ifndef DEBUG_H_
#define DEBUG_H_
#ifndef NDEBUG

#include "soft_uart.h"
#include <avr/delay.h>

/**
 * \brief Initializes the soft uart and prints a hello message
 */
#define DEBUG_INIT do{ \
		soft_uart_init(); \
		soft_uart_send('H'); \
		soft_uart_send('i'); \
		soft_uart_send('!'); \
	}while(0)

/**
 * \brief Prints a binary debug message
 * \details The first arguments is a debug id and the second an appended value.
 */
// TODO: remove delay!
#define DEBUG_PRINT(id,val) do{ \
		soft_uart_send(0xAA); \
		soft_uart_send((id)); \
		soft_uart_send((val)); \
		_delay_us(50); \
	}while(0)

/**
 * \brief Prints a variable length start sequence
 * \details The first argument is the message identifier
 */
#define DEBUG_PRINT_START(id) do{ \
		soft_uart_send(0x55); \
		soft_uart_send((id)); \
	}while(0)

/**
 * \brief Prints the val byte
 * \details It is advised to start the variable length debug sequence by calling
 * DEBUG_PRINT_START.
 */
#define DEBUG_BYTE(val) \
	soft_uart_send((val))

#else // --------------- Dummy functions ---------------------

#define DEBUG_INIT while(0)
#define DEBUG_PRINT(id,val) while(0)
#define DEBUG_PRINT_START(id) while(0)
#define DEBUG_BYTE(val) while(0)

#endif

#endif /* DEBUG_H_ */
