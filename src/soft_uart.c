/**
 * \file soft_uart.c
 * \brief implements the software UA(R)T (transmission only)
 * \details The actual baud rate depends on the chips frequency. At 8MHz a baud
 * rate of approximately 115.2kHz is achieved. A transmission scheme of 8N1 is
 * maintained. The module occupies the following hardware:
 * <ul>
 * 	<li>PB6 as TxD</li>
 * </ul>
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

#include "soft_uart.h"

#include <avr/io.h>
#include <util/delay.h>
#include <util/atomic.h>

/** \brief The transmission port register */
#define SOFT_UART_TXPORT (PORTB)
/** \brief The transmission DDR register */
#define SOFT_UART_TXDDR (DDRB)
/** \brief The transmission pin number */
#define SOFT_UART_TXNR (PORTB6)

void soft_uart_init(void) {
	SOFT_UART_TXPORT |= _BV(SOFT_UART_TXNR);
	SOFT_UART_TXDDR |= _BV(SOFT_UART_TXNR);
	_delay_us(87); // Wait until the spurious byte is transmit
}

void soft_uart_send(uint8_t data) {

	// One bit: 72 Cycles as HW UART (instead of 69 Cycles)
	ATOMIC_BLOCK(ATOMIC_RESTORESTATE)
	{
		asm volatile (
				// Initialize shifted bit loop counter
				"ldi r18, 0xFF                \n\t"
				// Start bit
				"cbi %[port], %[pin]          \n\t"// 2 Cycles
				"rcall soft_uart_wait_bit     \n\t"// 61 Cycles
				"push r18                     \n\t"// 2 Cycles |-> 4 Cycles filler
				"pop r18                      \n\t"// 2 Cycles |

				// Data bits
				"soft_uart_bitloop:           \n\t"
				"  in r19, %[port]            \n\t"// 1 Cycle
				"  sbrc %[data], 0            \n\t"// 1/2 Cycles  |-> Always 2 Cycles
				"    sbr r19, %[pinmsk]       \n\t"// 1 Cycle     |
				"  sbrs %[data], 0            \n\t"// 1/2 Cycles
				"    cbr r19, %[pinmsk]       \n\t"// 1 Cycle
				"  out %[port], r19           \n\t"// 1 Cycle
				"  lsr %[data]                \n\t"// 1 Cycle
				"  rcall soft_uart_wait_bit   \n\t"// 72-10 Cycles
				"  lsr r18                    \n\t"// 1 Cycle
				"  brne soft_uart_bitloop     \n\t"// 1/2 Cycles

				// Stop bit (Use necessary jump to delay the operation)
				"rjmp soft_uart_end           \n\t"// 2 Cycles

				// wait_bit: 61 Cycles
				"soft_uart_wait_bit:          \n\t"// RCALL: 4  Cycles
				"  push r18                   \n\t"// 2 Cycles
				"  ldi r18, 16                \n\t"// 1 Cycle
				"soft_uart_waitloop:          \n\t"// -- (Loop: 3*16-1 Cycles) --
				"    dec r18                  \n\t"// 1 Cycle
				"    brne soft_uart_waitloop  \n\t"// 1/2 Cycles
																					 // ---------------------------
				"  pop r18                    \n\t"// 2 Cycles
				"  nop                        \n\t"// 1 Cycle
				"  ret                        \n\t"// 5 Cycles

																					 // Finalize the stop bit
				"soft_uart_end:               \n\t"
				"push r18                     \n\t"// 2 Cycles |-> 4 Cycles filler
				"pop r18                      \n\t"// 2 Cycles |
				"sbi %[port], %[pin]          \n\t"// 2 Cycles
				"rcall soft_uart_wait_bit     \n\t"

				:																	 // Output operands:
				[data] "+a" (data)
				:																	 // Input operands:
				[port] "I" (_SFR_IO_ADDR(SOFT_UART_TXPORT)),
				[pin] "I" (SOFT_UART_TXNR),
				[pinmsk] "M" (_BV(SOFT_UART_TXNR)),
				"0" (data)												 // Also input operand
				:																	 // Clobber list:
				"r18","r19"
		);
	}
}
