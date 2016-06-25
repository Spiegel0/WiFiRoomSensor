/**
 * \file soft_uart.h
 * \brief Specifies a simple bit banging UART which may be used for debugging
 * purpose.
 * \details The UART operates on a fixed frequency and directly drives an IO
 * port. Currently, only the transmission and no reception is implemented.
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
#ifndef SOFT_UART_H_
#define SOFT_UART_H_

#include <stdint.h>

/**
 * \brief Initializes the module
 * \details The function has to be called before any other function of the
 * module is accessed.
 */
void soft_uart_init(void);

/**
 * \brief Sends the given byte
 * \param data The byte to transmit.
 */
void soft_uart_send(uint8_t data);

#endif /* SOFT_UART_H_ */
