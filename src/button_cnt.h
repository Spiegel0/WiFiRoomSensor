/**
 * \file button_cnt.h
 * \brief The file specifies an interface for a simple button counter.
 * \details The module maintains an internal counter which may be increased and
 * decreased by external buttons. Additionally, a third button is evaluated
 * which may generate further events.
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
#ifndef BUTTON_CNT_H_
#define BUTTON_CNT_H_

#include <stdint.h>

/**
 * \brief Function type which is used to notify the callee
 * \details The function is used to indicate state changes initiated by a user
 * interaction.
 * \param cnt The current internal counter value
 * \param btn A bit mask of the buttons which trigger the event. The LSB is
 * associated with the "ok" button, the second bit is associated with the "up"
 * button and the third bit is associated with the "down" button.
 */
typedef void (*button_cnt_callback_t)(int16_t cnt, uint8_t btn);

/**
 * \brief Initializes the module and registers the callback function.
 * \details The function has to be called before any other function is used. The
 * registered callback is executed in a non-interrupt context only. It is called
 * as soon as a button is pressed.
 * \param callback The callback function which indicates a user action.
 */
void button_cnt_init(button_cnt_callback_t callback);

/**
 * \brief The time handler function which needs to be called whenever the fast
 * system timer fires.
 */
void button_cnt_timedFastTick(void);

/**
 * \brief Returns the current state of the counter variable.
 * \return The current state of the counter variable.
 */
uint16_t button_cnt_getCounter(void);

#endif /* BUTTON_CNT_H_ */
