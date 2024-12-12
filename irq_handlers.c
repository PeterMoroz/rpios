#include "irq_handlers.h"
#include "interrupts.h"
#include "core_timer.h"
#include "oneshot_timer.h"
#include "thread.h"

#define _TRACE_PRINT_

#ifdef _TRACE_PRINT_
#include "printf.h"
#endif

#include <stdint.h>

#define CORE0_IRQ_SOURCE ((volatile uint32_t *)0x40000060)
#define AUX_MU_IIR       ((volatile uint32_t *)0x3F215048)

void handle_irq_el0_64()
{
	uart_puts("handle_irq_el0_64\r\n");
}

void handle_irq_el1h()
{
	uint32_t core0_irq_source = *CORE0_IRQ_SOURCE;
	if (core0_irq_source & 0x2) {
#ifdef _TRACE_PRINT_
		uint32_t seconds = core_timer_get_seconds();
		core_timer_irq_handler();
		printf("core timer 0: %x\n", seconds);
#endif
		oneshot_timer_tick();
		thread_sched_tick();
	} else if (core0_irq_source & 0x100) {
		uint32_t aux_irq_status = *AUX_MU_IIR;
		if ((aux_irq_status & 0x6) == 0x2) {
			uart_handle_tx_irq();
		} else if ((aux_irq_status & 0x6) == 0x4) {
			uart_handle_rx_irq();
		}
	}
}

