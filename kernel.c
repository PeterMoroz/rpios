#include "uart.h"
#include "mbox.h"

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
	char c;
	char cmd[8] = { '\0' };
	int i = 0;
	int rst = 0;

	uart_init();

	print_board_sn();
	print_board_revision();

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
			if (rst) {
				cancel_reset();
				rst = 0;
				uart_puts("reset canceled\n");
			} else {
				uart_puts("schedule reset\n");
				schedule_reset(1000000);
				rst = 1;
			}

		} else {
			uart_puts("unknown command\n");
		}
	}
}
