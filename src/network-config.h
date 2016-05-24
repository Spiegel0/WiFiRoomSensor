/*
 * \file network-config.h
 * \brief The file contains the default network configuration
 * \details Most parameters may be defined externally by passing the appropriate
 * macros or by editing the current file.
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
#ifndef NETWORK_CONFIG_H_
#define NETWORK_CONFIG_H_

/** \brief The port number of the opened server */
#define NW_CONFIG_SRV_PORT "61499"

/** \brief The destination network name */
#define NW_CONFIG_NETWORK "Elysion"

/**
 * \brief The network password
 */
#ifndef NW_CONFIG_PWD
	#include "pwd.txt"
#endif

#endif /* NETWORK_CONFIG_H_ */
