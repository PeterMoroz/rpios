#include "gpio.h"
#include "uart.h"
#include "ring_buffer.h"

#define AUX_ENABLE     ((volatile uint32_t*)(MMIO_BASE + 0x00215004))
#define AUX_MU_IO     ((volatile uint32_t*)(MMIO_BASE + 0x00215040))
#define AUX_MU_IER     ((volatile uint32_t*)(MMIO_BASE + 0x00215044))
#define AUX_MU_IIR     ((volatile uint32_t*)(MMIO_BASE + 0x00215048))
#define AUX_MU_LCR     ((volatile uint32_t*)(MMIO_BASE + 0x0021504C))
#define AUX_MU_MCR     ((volatile uint32_t*)(MMIO_BASE + 0x00215050))
#define AUX_MU_LSR     ((volatile uint32_t*)(MMIO_BASE + 0x00215054))
#define AUX_MU_MSR     ((volatile uint32_t*)(MMIO_BASE + 0x00215058))
#define AUX_MU_SCRATCH ((volatile uint32_t*)(MMIO_BASE + 0x0021505C))
#define AUX_MU_CNTL ((volatile uint32_t*)(MMIO_BASE + 0x00215060))
#define AUX_MU_STAT ((volatile uint32_t*)(MMIO_BASE + 0x00215064))
#define AUX_MU_BAUD ((volatile uint32_t*)(MMIO_BASE + 0x00215068))

static ring_buffer_t tx_buffer;
static ring_buffer_t rx_buffer;

void uart_init()
{
	ring_buffer_init(&rx_buffer);
	ring_buffer_init(&tx_buffer);

	register uint32_t r;

	*AUX_ENABLE |= 1;
	*AUX_MU_CNTL = 0;
	*AUX_MU_LCR = 3;
	*AUX_MU_MCR = 0;
	*AUX_MU_IER = 0;
	// *AUX_MU_IER = 0xF;
	*AUX_MU_IIR = 6;
	*AUX_MU_BAUD = 270;
	r = *GPFSEL1;
	r &= ~((7 << 12) | (7 << 15));
	r |= (2 << 12) | (2 << 15);
	*GPFSEL1 = r;
	*GPPUD = 0;
	r = 150;
	while (r--) { asm volatile("nop"); }
	*GPPUDCLK0 = (1 << 14) | (1 << 15);
	r = 150;
	while (r--) { asm volatile("nop"); }
	*GPPUDCLK0 = 0;
	*AUX_MU_CNTL = 3;
}

void uart_putc(char c)
{
	do { asm volatile("nop"); } while (!(*AUX_MU_LSR & 0x20));
	*AUX_MU_IO = c;
}

char uart_getc()
{
	char r;
	do { asm volatile("nop"); } while(!(*AUX_MU_LSR & 0x01));
	r = (char)(*AUX_MU_IO);
	return r;
}

void uart_puts(const char *s)
{
	while (*s) {
		if (*s == '\n')
			uart_putc('\r');
		uart_putc(*s++);
	}
}

void uart_put_uint32_hex(uint32_t x) 
{
	uint32_t n;
	int c;
	for (c = 28; c >= 0; c -= 4) {
		n = (x >> c) & 0xF;
		n += n > 9 ? 0x37 : 0x30;
		uart_putc(n);
	}
}

void uart_put_uint64_hex(uint64_t x) 
{
	uint64_t n;
	int c;
	for (c = 60; c >= 0; c -= 4) {
		n = (x >> c) & 0xF;
		n += n > 9 ? 0x37 : 0x30;
		uart_putc(n);
	}
}

void uart_read_rx_fifo()
{
	char c = (char)(*AUX_MU_IO);
	ring_buffer_put(&rx_buffer, c);
}

void uart_write_tx_fifo()
{
	char c;
	if (ring_buffer_get(&tx_buffer, &c))
		*AUX_MU_IO = c;
}

void uart_putc_async(char c)
{
	if (!ring_buffer_is_full(&tx_buffer))
		ring_buffer_put(&tx_buffer, c);
}

char uart_getc_async()
{
	char c = ' ';
	ring_buffer_get(&rx_buffer, &c);
	return c;
}

