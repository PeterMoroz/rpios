#ifndef __UART_H__
#define __UART_H__

void uart_init();
void uart_send(unsigned char c);
char uart_getc();
void uart_puts(char *s);
void uart_hex(unsigned char x);

#endif
