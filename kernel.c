#include "uart.h"
#include "mbox.h"
#include "cpio.h"
#include "utils.h"
#include "strutils.h"

#define PM_PASSWORD 0x5a000000
#define PM_RSTC ((volatile unsigned int *)0x3F10001c)
#define PM_WDOG ((volatile unsigned int *)0x3F100024)

void schedule_reset(int tick) 
{
	*PM_RSTC = PM_PASSWORD | 0x20;
	*PM_WDOG = PM_PASSWORD | tick;
}

void cancel_reset() 
{
	*PM_RSTC = PM_PASSWORD;
	*PM_WDOG = PM_PASSWORD;
}

void readline(char *buf, int maxlen) {
       int n = 0;
       while (n < maxlen - 1) {
	       char c = uart_getc();
	       if (c == '\n' || c == '\r' || c == '\0')
		       break;
	       buf[n++] = c;
       }
       buf[n] = '\0';
}

unsigned char crc8(const char *data, int size) {
	unsigned char crc = 0;
	for (int i = 0; i < size; i++)
		crc += data[i];
	return crc;
}

int crc16(const char *data, int size) {
	int crc = 0;
	unsigned char i = 0;

	while (--size >= 0) {
		crc = crc ^ (int)*data++ << 8;
		i = 8;
		do {
			if (crc & 0x8000)
				crc = crc << 1 ^ 0x1021;
			else
				crc = crc << 1;
		} while (--i);
	}
	return crc;
}

void load_kernel() {
	char c;
	while (1) {
		uart_send(0x43);
		c = uart_getc();
		if (c == 0x00)
			return;
		if (c == 0x01)
			break;
	}

	// not a packet header, just return
	if (c != 0x01)
		return;

	unsigned short packet_num = 1;
	char data_buffer[128];
	char *kernel = (char *)0x00;
	// receiving kernel image
	while (1) {
		switch (c) {
			case 0x01: { // SOH - Start Of Header
				char n1 = uart_getc();
				char n2 = uart_getc();
				for (int i = 0; i < 128; i++) {
					data_buffer[i] = uart_getc();
				}
				char c1 = uart_getc();
				char c2 = uart_getc();

				if ((unsigned short)n1 + (unsigned short)n2 != 0xFF) {
					uart_send(0x15);
				} else if ((unsigned short)n1 != packet_num) {
					uart_send(0x15);
				} else {
					// TO DO: fix CRC check failed.
					/*
					int crc = c1;
					crc = crc << 8;
					crc |= c2 & 0xFF;
					if (crc != crc16(data_buffer, 128)) {
						uart_send(0x15);
					} else {
						// TO DO: write piece of kernel
						packet_num++;
						uart_send(0x06);
					}
					*/
					unsigned char crc = crc8(data_buffer, 128);
					if (crc != (unsigned char)c1 || crc != (unsigned char)c2) {
						uart_send(0x15);
					} else {
						for (int i = 0; i < 128; i++) {
							*kernel++ = data_buffer[i];
						}
						packet_num++;
						uart_send(0x06);
					}
				}
			}
			break;
			case 0x04: { // EOT - End Of Transmission
				uart_send(0x06);
			}
			break;
			case 0x17: { // ETB - End of Transmission Block
				uart_send(0x06);
				branch_to_address((void *)0x00);
				// return;
			}
			break;
		default:
			return;
		}
		c = uart_getc();
	}
}


void print_board_sn()
{
	volatile unsigned __attribute__((aligned(16))) mbox[8];
	mbox[0] = 8 * 4;
	mbox[1] = MBOX_REQUEST;
	mbox[2] = MBOX_TAG_GETSERIAL;
	mbox[3] = 8;
	mbox[4] = 0;
	mbox[5] = 0;
	mbox[6] = 0;
	mbox[7] = MBOX_TAG_LAST;

	unsigned data = (unsigned long)&mbox[0] >> 4;
	unsigned mail = mbox_compose(MBOX_CH_PROP, data);
	mbox_put(mail);
	while (1) {
		if (mail == mbox_get())
			break;
	}

	if (mbox[1] != MBOX_RESPONSE_SUCCESS) {
		uart_puts("can't read board S/N\n");
	} else {
		uart_puts("The board S/N: ");
		uart_hex(mbox[6]);
		uart_hex(mbox[5]);
		uart_send('\r');
		uart_send('\n');
	}
}

void print_board_revision()
{
	volatile unsigned __attribute__((aligned(16))) mbox[7];
	mbox[0] = 7 * 4;
	mbox[1] = MBOX_REQUEST;
	mbox[2] = MBOX_TAG_GETREVISION;
	mbox[3] = 4;
	mbox[4] = 0;
	mbox[5] = 0;
	mbox[6] = MBOX_TAG_LAST;

	unsigned data = (unsigned long)&mbox[0] >> 4;
	unsigned mail = mbox_compose(MBOX_CH_PROP, data);
	mbox_put(mail);
	while (1) {
		if (mail == mbox_get())
			break;
	}

	if (mbox[1] != MBOX_RESPONSE_SUCCESS) {
		uart_puts("can't read board revision\n");
	} else {
		uart_puts("The board revision: ");
		uart_hex(mbox[5]);
		uart_send('\r');
		uart_send('\n');
	}
}

void kmain(void)
{
	int rst = 0;
	char cmd[32];

	uart_init();
	load_kernel();

	print_board_sn();
	print_board_revision();

	while (1) {
		uart_send('>');
		uart_send(' ');
		
		readline(cmd, 32);
		uart_puts(cmd);
		uart_send('\r');
		uart_send('\n');
		
		if (strcmp(cmd, "help") == 0) {
			uart_puts("help   : print this help menu\n");
			uart_puts("hello  : print Hello World!\n");
			uart_puts("reboot : reboot the device\n");
		} else if (strcmp(cmd, "hello") == 0) {
			uart_puts("Hello World!\n");
		} else if (strcmp(cmd, "reboot") == 0) {
			if (rst) {
				cancel_reset();
				rst = 0;
				uart_puts("reset canceled\n");
			} else {
				uart_puts("schedule reset\n");
				schedule_reset(1000000);
				rst = 1;
			}

		} else if (strcmp(cmd, "ls") == 0) {
			cpio_read_catalog();
		} else if (strncmp(cmd, "cat", 3) == 0) {
			const char *fname = cmd + 3;
			while (*fname && *fname == ' ')
				fname++;
			if (*fname) {
				if (cpio_read_file(fname) != 0)
					uart_puts("file not found!\r\n");
			}
		} else {
			uart_puts("unknown command\n");
		}
	}
}
