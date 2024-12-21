#include "printf.h"

#include <stdint.h>

#include "syscall.h"
#include "strutils.h"

static void print_symbol(char s)
{
	uart_write(&s, 1);
}

static void print_string(const char *s)
{
	const size_t n = strlen(s);
	uart_write(s, n);
}

static void print_uint32_hex(uint32_t x)
{
	char strbuff[8] = { '0' };
	int i = 0;
	for (int c = 28; c >= 0; c -= 4, i++) {
		uint32_t n = (x >> c) & 0xF;
		n += n > 9 ? 0x37 : 0x30;
		strbuff[i] = n & 0xFF;
	}
	uart_write(strbuff, sizeof(strbuff));
}

static void print_uint64_hex(uint64_t x)
{
	char strbuff[8] = { '0' };
	int i = 0;
	for (int c = 28; c >= 0; c -= 4, i++) {
		uint64_t n = (x >> c) & 0xF;
		n += n > 9 ? 0x37 : 0x30;
		strbuff[i] = n & 0xFF;
	}
	uart_write(strbuff, sizeof(strbuff));
}

void printf(const char *format, ...)
{
	va_list args;
	va_start(args, format);

	for (; *format != '\0'; format++) {
		if (*format == '%') {
			switch (*(++format)) {
				case '%':
					print_symbol('%');
					break;
				case 'x':
					print_uint32_hex(va_arg(args, uint32_t));
					break;
				case 'X':
					print_uint64_hex(va_arg(args, uint64_t));
					break;
				case 's':
					print_string(va_arg(args, char *));
					break;
			}
		} else {
			print_symbol(*format);
		}
	}
	va_end(args);
}

