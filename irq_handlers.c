#include "irq_handlers.h"
#include "uart.h"
#include "interrupts.h"
#include "core_timer.h"

#include <stdint.h>

#define CORE0_IRQ_SOURCE ((volatile uint32_t *)0x40000060)

void handle_irq_el0_64()
{
	uart_puts("handle_irq_el0_64\r\n");
}

void handle_irq_el1h()
{
	// asm volatile("msr daifset, #2");
	// uart_puts("handle_irq_el1h: ");
	uint32_t core0_irq_source = *CORE0_IRQ_SOURCE;
	// uart_put_uint32_hex(core0_irq_source);
	// uart_putc('\r');
	// uart_putc('\n');
	if (core0_irq_source & 0x2) {
		uint32_t seconds = core_timer_get_seconds();
		//uint32_t freq = 0;
		//uint32_t cval = 0;
		//asm volatile("mrs %0, cntfrq_el0" : "=r" (freq));
		//asm volatile("mrs %0, cntpct_el0" : "=r" (cval));
		//uint32_t seconds = cval / freq;
		core_timer_irq_handler();
		uart_puts("core timer 0: ");
		uart_put_uint32_hex(seconds);
		//uart_put_uint32_hex(cval);
		//uart_putc('\t');
		//uart_put_uint32_hex(freq);
		uart_putc('\r');
		uart_putc('\n');
	}
	// asm volatile("msr daifclr, #2");
}

