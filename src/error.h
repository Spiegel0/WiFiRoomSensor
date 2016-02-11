/**
 * \file error.h
 * \brief Defines some commonly used error codes
 * \author Michael Spiegel, <michael.h.spiegel@gmail.com>
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

#ifndef ERROR_H_
#define ERROR_H_

/**
 * \brief The enumeration defines a status type
 * \details The enumeration encodes several different statii. Only the first
 * state, <code>success</code>, corresponds to a successfully completed operation.
 */
typedef enum {
	success = 0, //< \brief The operation was finished successfully
	err_chksum, //< \brief The received checksum was invalid
	err_noSignal, //< \brief No signal was received.
	err_invalidChannel, //< \brief The requested channel does not exist
	err_status //< \brief The status message is not ok
} status_t;

#endif /* ERROR_H_ */
