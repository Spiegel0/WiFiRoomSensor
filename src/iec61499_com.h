/**
 * \file iec61499_com.h
 * \brief Defines functions which encode and decode ASN.1-based messages from an
 * IEC 61499 compliant controller.
 * \details The module is state-less and operates on external message buffers
 * only. The encoding follows the informative annex a of the IEC 61499-1 and the
 * implementation of the FBDK and 4DIAC FORTE.
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
#ifndef IEC61499_COM_H_
#define IEC61499_COM_H_

#include <stdint.h>

/** \brief The number of bytes which are allocated by an encoded INT value */
#define IEC61499_COM_INT_ENC_SIZE (3)

/**
 * \brief Adds an INT value to the message buffer.
 * \details If the total size of the buffer is too small, it will not be
 * accessed. After the function is called, the nextIndex will be increased by
 * the size of the encoded value, regardless of the size of the buffer.
 * \param buffer A pointer to the first byte of the destination message buffer.
 * It is assumed that the buffer contains at least size bytes.
 * \param size The total capacity of the message buffer in bytes
 * \param nextIndex A pointer to a memory location which holds the next index in
 * the buffer which should be populated. The location is increased by the size
 * of the target encoding.
 * \param value The actual value which should be encoded.
 */
void iec61499_com_encodeINT(uint8_t *buffer, uint8_t size, uint8_t *nextIndex,
		int16_t value);

#endif /* IEC61499_COM_H_ */
