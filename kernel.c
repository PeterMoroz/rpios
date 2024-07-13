#include "uart.h"

void kmain(void)
{
	char c;
	uart_init();
	uart_puts("Hello World\n");
	while (1) {
		c = uart_getc();
		if (c == '\r') {
			uart_send('\r');
			uart_send('\n');
		} else {
			uart_send(c);
		}
	}
}
