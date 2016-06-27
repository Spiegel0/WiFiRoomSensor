/**
 * \file oscillator.c
 * \brief Implements the oscillator module
 * \details The module sets the oscillator calibration. It requires the build
 * system to alter certain variables after the code has been generated. In
 * particular, the variable \ref oscillator_calibration has to be set to the
 * values read by the programmer. These values cannot be accessed by the chip
 * itself. Consider using gdb to alter the default values in the ELF file. The
 * following command turns out to be particular useful:
 * <code> gdb -batch -x commands.batch --write binary.elf </code> with the
 * command-file: <code>set var (char[4]) oscillator_calibration = { 0xbb,0xbb,
 * 0xb6,0xb5 }</code>
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

#include "oscillator.h"

#include <stdint.h>
#include <avr/io.h>
#include <avr/eeprom.h>

/**
 * \brief The OSCAL values for 1.0MHz, 2.0MHz, 4.0MHz, 8.0MHz
 * \details These values have to be altered after the build process when the
 * destination chip is known.
 */
uint8_t oscillator_calibration[] EEMEM = { 0x00, 0x00, 0x00, 0x00 };

#if F_CPU == 1000000UL
/**
 * \brief The index of the correct OSCAL value in \ref oscillator_calibration
 */
#define OSCILLATOR_F_INDEX (0)
#elif F_CPU == 2000000UL
/**
 * \brief The index of the correct OSCAL value in \ref oscillator_calibration
 */
#define OSCILLATOR_F_INDEX (1)
#elif F_CPU == 4000000UL
/**
 * \brief The index of the correct OSCAL value in \ref oscillator_calibration
 */
#define OSCILLATOR_F_INDEX (2)
#elif F_CPU == 8000000UL
/**
 * \brief The index of the correct OSCAL value in \ref oscillator_calibration
 */
#define OSCILLATOR_F_INDEX (3)
#else
/**
 * \brief The index of the correct OSCAL value in \ref oscillator_calibration
 */
#define OSCILLATOR_F_INDEX (0)
#warning "F_CPU is not set to a valid frequency for the internal RC oscillator"
#endif

void oscillator_init(void) {
	OSCCAL = eeprom_read_byte(&oscillator_calibration[OSCILLATOR_F_INDEX]);
}
