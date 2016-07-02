/**
 * \file ws2801.h
 * \brief Specifies the communication interface for the ES2801 LED driver.
 * \details Before any other function of the module can be used,
 * \ref ws2801_init has to be called. Additionally, the module requires the
 * \ref ws2801_timedTick function to be called in the interval of the system
 * timer. The content of the chained LEDs are internally buffered and allow to
 * access a single value.
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
#ifndef WS2801_H_
#define WS2801_H_

#include "error.h"

#include <stdint.h>

/** \brief The maximum number of controlled LEDs */
#define WS2801_CHAIN_SIZE (10)

/**
 * \brief Initializes the module
 * \details The function has to be called before any other function is called.
 * It is assumed that interrupts are globally disabled when calling the
 * initialization function.
 */
void ws2801_init(void);

/**
 * \brief Maintains periodic tasks
 * \details The function has to be called whenever the system timer fires.
 */
void ws2801_timedTick(void);

/**
 * \brief Sets the the color at the given position in the internal buffer.
 * \details The function does not directly update the status of the LEDs. In
 * order to update the LED chain, \ref ws2801_update has to be called. If the
 * position value is bigger or equal than WS2801_CHAIN_SIZE, all elements of the
 * buffer will be set to the same value. If the module is currently updating the
 * LED chain, the function will fail and nothing will be set.
 * \param position The buffer position between 0 (inclusive) and
 * WS2801_CHAIN_SIZE (exclusive) or a value which indicates that all pixels
 * should be set to the same value.
 * \param red The value of the red channel.
 * \param green The value of the green channel.
 * \param blue The value of the blue channel.
 * \return The status of the operation. It will be successful if and only if
 * the value was set.
 */
status_t ws2801_setValue(uint8_t position, uint8_t red, uint8_t green,
		uint8_t blue);

/**
 * \brief Updates the status of the LEDs according to the internal buffer.
 * \details The function will update the values in a background task. If the
 * module is still busy with updating the last value, an error will be returned.
 * \returns The status of the operation.
 */
status_t ws2801_update(void);

#endif /* WS2801_H_ */
