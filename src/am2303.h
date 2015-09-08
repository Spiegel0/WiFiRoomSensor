/**
 * am2303.h
 * \brief Reads the temperature and humidity value of a DHT22/am3203 sensor
 * \details The module implements an interrupt driven decoding function. It is
 * capable of decoding a single sensor at once. After a read cycle another
 * sensor might be queried. After the values are read or on error a callback
 * function is called.
 * \author Michael Spiegel, <michael.h.spiegel@gmail.com>
 */

#ifndef AM2303_H_
#define AM2303_H_

#include <stdint.h>

#include "error.h"

/**
 * \brief Defines a callback function type which indicates a completed read
 * cycle
 * \details The function may be called in an interrupt context. If an only if
 * the given status is <code>success</code>, the temperature and humidity variables
 * will contain valid values. The channel parameter indicates the requested
 * channel number.
 * \param status The status of the operation
 * \param temperature The received temperature value. If the status is not
 * successful, it may contain arbitrary content.
 * \param humidity The received humidity value. If the status is not successful,
 * it may contain arbitrary content.
 * \param channel The requested channel
 *
 * Copyright (C) 2015 Michael Spiegel
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

typedef void (* am2303_readDone_t)(status_t status, uint16_t temperature,
		uint16_t humidity, uint8_t channel);

/**
 * \brief Initializes the module
 * \details The function must be called before calling any other function. It is
 * assumed that global interrupts are not enabled.
 */
void am2303_init(void);

/**
 * \brief Requests a sensor's values
 * \details The function will start the decoding cycle. It must be called at
 * maximum once before the given callback function is invoked. After the request
 * was processed, the callback function is invoked. It is assumed that
 * interrupts are globally enabled. The caller must assure that the function is
 * called at maximum one every two seconds. It is advised to keep a five seconds
 * interval between two consecutive measurements.
 * \param channel The channel identifier. Currently, only two channels are
 * supported.
 * \param callback The callback function which indicates a completed request.
 * The callback function may be invoked in an interrupt context. Additionally,
 * global interrupts may be disabled.
 */
void am2303_startReading(uint8_t channel, am2303_readDone_t callback);

#endif /* AM2303_H_ */
