#include "uart.h"
#include "utils.h"

#include <stdint.h>

const char *descriptions[] = 
{
	"SYNCHRONOUS_EL1T",
	"IRQ_EL1T",
	"FIQ_EL1T",
	"SERROR_EL1T",

	"SYNCHRONOUS_EL1H",
	"IRQ_EL1H",
	"FIQ_EL1H",
	"SERROR_EL1H",

	"SYNCHRONOUS_EL0_64",
	"IRQ_EL0_64",
	"FIQ_EL0_64",
	"SERROR_EL0_64",

	"SYNCHRONOUS_EL0_32",
	"IRQ_EL0_32",
	"FIQ_EL0_32",
	"SERROR_EL0_32",

	"error when handle svc (64-bit)",
	"syscall error",
};

void print_exception_info(uint32_t id, uint64_t spsr, uint64_t esr, uint64_t elr)
{
	uart_puts("exception: ");
	uart_puts(descriptions[id]);
	uart_puts(" SPSR: ");
	uart_put_uint64_hex(spsr);
	uart_puts(" ESR: ");
	uart_put_uint64_hex(esr);
	uart_puts(" ELR: ");
	uart_put_uint64_hex(elr);
	uart_puts(" ---- \r\n");
}

void print_exception_info2(uint32_t id, uint64_t spsr, uint64_t esr, uint64_t elr, uint64_t r24, uint64_t r25)
{
	uart_puts("exception: ");
	uart_puts(descriptions[id]);
	uart_puts(" SPSR: ");
	uart_put_uint64_hex(spsr);
	uart_puts(" ESR: ");
	uart_put_uint64_hex(esr);
	uart_puts(" ELR: ");
	uart_put_uint64_hex(elr);
	uart_puts(" x24: ");
	uart_put_uint64_hex(r24);
	uart_puts(" x25: ");
	uart_put_uint64_hex(r25);
	uart_puts(" ---- \r\n");
}


void print_current_el()
{
	uint64_t el;
	
	// read the current level from system register
	asm volatile ("mrs %0, CurrentEL" : "=r" (el));
	uart_puts("current el: ");
	uart_put_uint64_hex((el >> 2) & 0x3);
}

