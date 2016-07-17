/**
 * \file system_timer.h
 * \brief The header file specifies a global system timer and its interface
 * \details The system timer uses a dedicated hardware timer. It enables the
 * usage of the hardware timer in several modules which do not require high
 * precision timings. The module provides an interface which may be polled
 * periodically. A fast channel may be used to trigger frequent timing events.
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
#ifndef SYSTEM_TIMER_H_
#define SYSTEM_TIMER_H_

#include <stdint.h>

/** \brief The period of the timer in milliseconds */
#define SYSTEM_TIMER_PERIOD_MS (197)

/** \brief The period of the fast timer in milliseconds */
#define SYSTEM_TIMER_FAST_PERIOD_MS (256UL * 128UL * 1000UL / F_CPU)

/**
 * \brief Converts a time interval in milliseconds to the number of timer ticks.
 * \details The number of ticks are rounded to the next higher integer value
 */
#define SYSTEM_TIMER_MS_TO_TICKS( time ) \
	(( (time) + SYSTEM_TIMER_PERIOD_MS - 1 ) / SYSTEM_TIMER_PERIOD_MS )

/**
 * \brief Converts a time interval in milliseconds to the number of timer ticks.
 * \details The number of ticks are rounded to the next higher integer value
 */
#define SYSTEM_TIMER_MS_TO_FAST_TICKS( time ) \
	(( (time) + SYSTEM_TIMER_FAST_PERIOD_MS - 1 ) / SYSTEM_TIMER_FAST_PERIOD_MS )


/**
 * \brief Initializes the system timer
 * \details The function has to be called before any other function is used. It
 * is expected that all interrupts are disabled when calling the function.
 */
void system_timer_init(void);

/**
 * \brief Queries the state of the timer and eventually resets it
 * \details If the timer is expired before the internal expired flag will be
 * reset. However, the function won't start the timer anew. The next indicated
 * event will not be influenced by the time system_timer_query() is executed. If
 * the timer was expired several times before the function is called, it will
 * only return one positive result.
 * \return Zero if no event was triggered, non-zero otherwise.
 */
uint8_t system_timer_query(void);

/**
 * \brief Queries the state of the fast timer and eventually resets it
 * \details If the timer is expired before the internal expired flag will be
 * reset. However, the function won't start the timer anew. The next indicated
 * event will not be influenced by the time system_timer_query() is executed. If
 * the timer was expired several times before the function is called, it will
 * only return one positive result.
 * \return Zero if no event was triggered, non-zero otherwise.
 */
uint8_t system_timer_queryFast(void);

#endif /* SYSTEM_TIMER_H_ */
