/*
 * \file iec61499_comm.c
 * \brief Implements the ASN.1 based encoding and decoding functionality.
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

#include "iec61499_com.h"

/** \brief Flags which indicate an application specific ASN.1 type class */
#define IEC61499_COM_CLASS_APPLICATION (0x40)

/**
 * \brief The ASN.1 tag number of INT without any flags
 * \details The tag numbers are defined in the informative Annex E of the IEC
 * 61499
 */
#define IEC61499_COM_TAG_INT (3)

void iec61499_com_encodeINT(uint8_t *buffer, uint8_t size, uint8_t *nextIndex,
		int16_t value) {

	if ((*nextIndex) + IEC61499_COM_INT_ENC_SIZE <= size) {
		buffer[*nextIndex] = IEC61499_COM_CLASS_APPLICATION | IEC61499_COM_TAG_INT;
		buffer[*nextIndex + 1] = (value >> 8) & 0xFF;
		buffer[*nextIndex + 2] = value & 0xFF;
	}

	*nextIndex += IEC61499_COM_INT_ENC_SIZE;

}
