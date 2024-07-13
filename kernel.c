#include "uart.h"

int strcmp(const char *s1, const char *s2)
{
	int r = -1;
	if (s1 == s2)
		return 0;

	if (s1 != 0 && s2 != 0) 
	{
		for (; *s1 == *s2; ++s1, ++s2)
		{
			if (*s1 == 0)
			{
				r = 0;
				break;
			}
		}

			if (r != 0)
				r = *(const char *)s1 - *(const char *)s2;
	}
	return r;
}

void kmain(void)
{
	char c;
	char cmd[8] = { '\0' };
	int i = 0;

	uart_init();
	while (1) {
		uart_send('>');
		uart_send(' ');
		i = 0;
		while (i < 7) {
			c = uart_getc();
			if (c == '\r' || c == '\n')
				break;
			cmd[i++] = c;
			uart_send(c);
		}

		cmd[i] = '\0';
		uart_send('\r');
		uart_send('\n');

		if (strcmp(cmd, "help") == 0) {
			uart_puts("help   : print this help menu\n");
			uart_puts("hello  : print Hello World!\n");
			uart_puts("reboot : reboot the device\n");
		} else if (strcmp(cmd, "hello") == 0) {
			uart_puts("Hello World!\n");
		} else if (strcmp(cmd, "reboot") == 0) {
			uart_puts("reboot not implemented yet\n");
		} else {
			uart_puts("unknown command\n");
		}
	}
}
