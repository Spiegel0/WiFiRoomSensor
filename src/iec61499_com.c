/**
 * \file iec61499_com.c
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

#include "esp8266_receiver.h"

/** \brief Flags which indicate an application specific ASN.1 type class */
#define IEC61499_COM_CLASS_APPLICATION (0x40)

/**
 * \brief The ASN.1 tag number of INT without any flags
 * \details The tag numbers are defined in the informative Annex E of the IEC
 * 61499
 */
#define IEC61499_COM_TAG_INT (3)
/**
 * \brief The ASN.1 tag number of USINT without any flags
 * \details The tag numbers are defined in the informative Annex E of the IEC
 * 61499
 */
#define IEC61499_COM_TAG_USINT (6)
/**
 * \brief The ASN.1 tag number of a BOOL TRUE value without any flags
 * \details The tag numbers are defined in the informative Annex E of the IEC
 * 61499
 */
#define IEC61499_COM_TAG_TRUE (1)
/**
 * \brief The ASN.1 tag number of a BOOL FALSE value without any flags
 * \details The tag numbers are defined in the informative Annex E of the IEC
 * 61499
 */
#define IEC61499_COM_TAG_FALSE (0)

void iec61499_com_encodeINT(uint8_t *buffer, uint8_t size, uint8_t *nextIndex,
		int16_t value) {

	if ((*nextIndex) + IEC61499_COM_INT_ENC_SIZE <= size) {
		buffer[*nextIndex] = IEC61499_COM_CLASS_APPLICATION | IEC61499_COM_TAG_INT;
		buffer[*nextIndex + 1] = (value >> 8) & 0xFF;
		buffer[*nextIndex + 2] = value & 0xFF;
	}

	*nextIndex += IEC61499_COM_INT_ENC_SIZE;

}

status_t iec61499_com_decodeUSINT(uint8_t rrbID, uint8_t size,
		uint8_t *nextIndex, uint8_t *value) {

	if (*nextIndex + IEC61499_COM_USINT_ENC_SIZE > size) {
		return err_indexOutOfBounds;
	}

	if (esp8266_receiver_getByte(rrbID, *nextIndex)
			!= (IEC61499_COM_TAG_USINT | IEC61499_COM_CLASS_APPLICATION)) {
		return err_invalidMagicNumber;
	}

	*value = esp8266_receiver_getByte(rrbID, *nextIndex + 1);
	*nextIndex += IEC61499_COM_USINT_ENC_SIZE;
	return success;
}

status_t iec61499_com_decodeBOOL(uint8_t rrbID, uint8_t size,
		uint8_t *nextIndex, uint8_t *value) {

	if (*nextIndex + IEC61499_COM_BOOL_ENC_SIZE > size) {
		return err_indexOutOfBounds;
	}

	if (esp8266_receiver_getByte(rrbID, *nextIndex)
			== (IEC61499_COM_TAG_TRUE | IEC61499_COM_CLASS_APPLICATION)) {

		*value = (uint8_t)(-1);

	}else if(esp8266_receiver_getByte(rrbID, *nextIndex)
			== (IEC61499_COM_TAG_FALSE | IEC61499_COM_CLASS_APPLICATION)){

		*value = 0;

	}else{
		return err_invalidMagicNumber;
	}

	*nextIndex += IEC61499_COM_BOOL_ENC_SIZE;
	return success;
}
