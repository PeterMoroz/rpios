#ifndef __UART_H__
#define __UART_H__

#include <stdint.h>

void uart_init();
void uart_send(char c);
char uart_getc();
void uart_puts(const char *s);
void uart_hex(uint32_t x);

#endif
