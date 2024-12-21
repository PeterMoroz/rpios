#include "kprintf.h"

#include <stdint.h>

#include "uart.h"

static void kprint_uint32_hex(uint32_t x)
{
	for (int c = 28; c >= 0; c -= 4) {
		uint32_t n = (x >> c) & 0xF;
		n += n > 9 ? 0x37 : 0x30;
		uart_putc(n);
	}
}

static void kprint_uint64_hex(uint64_t x)
{
	for (int c = 28; c >= 0; c -= 4) {
		uint64_t n = (x >> c) & 0xF;
		n += n > 9 ? 0x37 : 0x30;
		uart_putc(n);
	}
}

void kprintf(const char *format, ...)
{
	va_list args;
	va_start(args, format);

	for (; *format != '\0'; format++) {
		if (*format == '%') {
			switch (*(++format)) {
				case '%':
					uart_putc('%');
					break;
				case 'x':
					kprint_uint32_hex(va_arg(args, uint32_t));
					break;
				case 'X':
					kprint_uint64_hex(va_arg(args, uint64_t));
					break;
				case 's':
					uart_puts(va_arg(args, char *));
					break;
			}
		} else {
			uart_putc(*format);
		}
	}
	va_end(args);
}

