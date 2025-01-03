#ifndef __UART_H__
#define __UART_H__

#include <stdint.h>

void uart_init();
void uart_putc(char c);
char uart_getc();
void uart_puts(const char *s);
void uart_put_uint32_hex(uint32_t x);
void uart_put_uint64_hex(uint64_t x);

void uart_handle_rx_irq();
void uart_handle_tx_irq();

void uart_putc_async(char c);
char uart_getc_async();

#endif
