/**
 * \file oscillator.h
 * \brief Configures the internal RC oscillator
 * \details The module needs to be initialized in order to load the correct
 * calibration.
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
#ifndef OSCILLATOR_H_
#define OSCILLATOR_H_

/**
 * \brief Initializes the internal oscillator and calibrates its frequency
 * \details The function has to be called before any time critical code is
 * executed.
 */
void oscillator_init(void);

#endif /* OSCILLATOR_H_ */
