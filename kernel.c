#include "uart.h"
#include "mbox.h"
#include "cpio.h"
#include "heap.h"
#include "fdt.h"
#include "utils.h"
#include "strutils.h"
#include "memutils.h"
#include "interrupts.h"
#include "core_timer.h"
#include "oneshot_timer.h"
#include "printf.h"
#include "allocator.h"
#include "pool_allocator.h"
#include "thread.h"


#define PM_PASSWORD 0x5a000000
#define PM_RSTC ((volatile uint32_t *)0x3F10001c)
#define PM_WDOG ((volatile uint32_t *)0x3F100024)

#define USER_STACK_SIZE 512

extern void init_exception_table();

extern void userprogram();

void schedule_reset(uint32_t tick)
{
	*PM_RSTC = PM_PASSWORD | 0x20;
	*PM_WDOG = PM_PASSWORD | tick;
}

void cancel_reset()
{
	*PM_RSTC = PM_PASSWORD;
	*PM_WDOG = PM_PASSWORD;
}

void readline(char *buf, size_t maxlen) {
       size_t n = 0;
       while (n < maxlen - 1) {
	       char c = uart_getc();
	       if (c == '\n' || c == '\r' || c == '\0')
		       break;
	       buf[n++] = c;
       }
       buf[n] = '\0';
}

/* TODO: 
 * rewrite using clearly defined integer types
 */
unsigned char crc8(const char *data, int size) {
	unsigned char crc = 0;
	for (int i = 0; i < size; i++)
		crc += data[i];
	return crc;
}

/* TODO:
 * rewrite using clearly defined integer types
 */
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

/* TO DO:
 * rewrite using clearly defined integer types
 */
void load_kernel() {
	char c;
	while (1) {
		uart_putc(0x43);
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
					uart_putc(0x15);
				} else if ((unsigned short)n1 != packet_num) {
					uart_putc(0x15);
				} else {
					// TO DO: fix CRC check failed.
					/*
					int crc = c1;
					crc = crc << 8;
					crc |= c2 & 0xFF;
					if (crc != crc16(data_buffer, 128)) {
						uart_putc(0x15);
					} else {
						// TO DO: write piece of kernel
						packet_num++;
						uart_putc(0x06);
					}
					*/
					unsigned char crc = crc8(data_buffer, 128);
					if (crc != (unsigned char)c1 || crc != (unsigned char)c2) {
						uart_putc(0x15);
					} else {
						for (int i = 0; i < 128; i++) {
							*kernel++ = data_buffer[i];
						}
						packet_num++;
						uart_putc(0x06);
					}
				}
			}
			break;
			case 0x04: { // EOT - End Of Transmission
				uart_putc(0x06);
			}
			break;
			case 0x17: { // ETB - End of Transmission Block
				uart_putc(0x06);
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
	volatile uint32_t __attribute__((aligned(16))) mbox[8];
	mbox[0] = 8 * 4;
	mbox[1] = MBOX_REQUEST;
	mbox[2] = MBOX_TAG_GETSERIAL;
	mbox[3] = 8;
	mbox[4] = 0;
	mbox[5] = 0;
	mbox[6] = 0;
	mbox[7] = MBOX_TAG_LAST;

	uint32_t data = (uint64_t)&mbox[0] >> 4;
	uint32_t mail = mbox_compose(MBOX_CH_PROP, data);
	mbox_put(mail);
	while (1) {
		if (mail == mbox_get())
			break;
	}

	if (mbox[1] != MBOX_RESPONSE_SUCCESS) {
		uart_puts("can't read board S/N\n");
	} else {
		uart_puts("The board S/N: ");
		uart_put_uint32_hex(mbox[6]);
		uart_put_uint32_hex(mbox[5]);
		uart_putc('\r');
		uart_putc('\n');
	}
}

void print_board_revision()
{
	volatile uint32_t __attribute__((aligned(16))) mbox[7];
	mbox[0] = 7 * 4;
	mbox[1] = MBOX_REQUEST;
	mbox[2] = MBOX_TAG_GETREVISION;
	mbox[3] = 4;
	mbox[4] = 0;
	mbox[5] = 0;
	mbox[6] = MBOX_TAG_LAST;

	uint32_t data = (uint64_t)&mbox[0] >> 4;
	uint32_t mail = mbox_compose(MBOX_CH_PROP, data);
	mbox_put(mail);
	while (1) {
		if (mail == mbox_get())
			break;
	}

	if (mbox[1] != MBOX_RESPONSE_SUCCESS) {
		uart_puts("can't read board revision\n");
	} else {
		uart_puts("The board revision: ");
		uart_put_uint32_hex(mbox[5]);
		uart_putc('\r');
		uart_putc('\n');
	}
}

void list_files()
{
	cpio_read_catalog(&uart_putc);
}

void print_file(const char *fname)
{
	int fsize = cpio_file_size(fname);
	if (fsize < 0) {
		uart_puts("file not found\r\n");
		return;
	}

	char *buffer = malloc(fsize);
	memset(buffer, 0, fsize);
	if (cpio_read_file(fname, buffer, fsize) < 0) {
		uart_puts("could not read file\r\n");
		return;
	}

	for (int i = 0; i < fsize; i++) {
		uart_putc(buffer[i]);
	}
	uart_putc('\r');
	uart_putc('\n');
}

/*
void execute_userprogram() 
{
	uint64_t el = get_el();
	uart_puts("current EL: ");
	uart_put_uint32_hex(el);
	uart_putc('\r');
	uart_putc('\n');

	uint8_t *stack = malloc(USER_STACK_SIZE);
	uart_puts("switching to EL0 to execute userprogram\r\n");
	execute_in_el0(&userprogram, stack);
}
*/

void fdt_node_visit(const uint8_t *node)
{
	const char *s = (const char *)node;
	if (strcmp(s, "chosen") == 0) {
		const uint8_t *p = (const uint8_t *)s;
		p = (uint8_t *)(((int64_t)p + (4 - 1)) & -4);
		uint32_t tag = read_uint32_be(p);
		p += sizeof(uint32_t);
		while (tag != FDT_END_NODE) {
			if (tag == FDT_PROP) {
				uint32_t len = read_uint32_be(p);
				p += sizeof(uint32_t);
				uint32_t nameoff = read_uint32_be(p);
				p += sizeof(uint32_t);
				const char *name = fdt_get_string(nameoff);
				if (strcmp(name, "linux,initrd-start") == 0) {
					uint32_t initrd_start = read_uint32_be(p);
					cpio_set_initrd_start(initrd_start);
				}
				/*
				if (strcmp(name, "linux,initrd-end") == 0) {
					uint32_t initrd_end = read_uint32_be(p);
					uart_puts("initrd end: ");
					uart_put_uint32_hex(initrd_end);
					uart_putc('\r');
					uart_putc('\n');
				}
				*/
				p += len;
				p = (uint8_t *)(((int64_t)p + (4 - 1)) & -4);
			}
			tag = read_uint32_be(p);
			p += sizeof(uint32_t);
		}
	}
}

void on_timer(void* arg)
{
	// do nothing
}

void test_formated_print()
{
	printf("printf: print text without arguments\r\n");
	printf("printf: print text with percentage characters %% %% \r\n");
	printf("printf: print text string %s %s \r\n", "hello", "world");
	printf("printf: print 32-bit numbers %x %x \r\n", 0x01234567, 0x89ABCDEF);
	// TO DO: fix - 64-bit integers are truncated
	printf("printf: print 64-bit numbers %x %x \r\n", 0x01234567ABCDEF, 0xFEDCBA9876543210);
}

void test_and_trace_allocator()
{
	void *p1 = NULL, *p2 = NULL, *p3 = NULL, *p4 = NULL, *p5 = NULL, *p6 = NULL, *p7 = NULL, *p8 = NULL;

	//printf(" -- allocate block 32K \n");
	//void* p = allocate(32 * 1024);
	//printf(" -- release allocated block \n");
	//release(p);
	//printf(" ---------------------- \n");

	//printf(" -- allocate 2 block of 16K \n");
	//p1 = allocate(16 * 1024);
	//p2 = allocate(16 * 1024);
	//printf(" -- release allocated blocks \n)";
	//release(p2);
	//release(p1);
	//printf(" ---------------------- \n");

	//printf(" -- allocate 4 block of 8K \n");
	//p1 = allocate(8 * 1024);
	//p2 = allocate(8 * 1024);
	//p3 = allocate(8 * 1024);
	//p4 = allocate(8 * 1024);
	//printf(" -- release allocated blocks \n");
	//release(p4);
	//release(p3);
	//release(p2);
	//release(p1);
	//printf(" ---------------------- \n");

	//printf(" -- allocate 8 block of 4K \n");
	//p1 = allocate(4 * 1024);
	//p2 = allocate(4 * 1024);
	//p3 = allocate(4 * 1024);
	//p4 = allocate(4 * 1024);

	//p5 = allocate(4 * 1024);
	//p6 = allocate(4 * 1024);
	//p7 = allocate(4 * 1024);
	//p8 = allocate(4 * 1024);
	//printf(" -- release allocated blocks \n");
	//release(p8);
	//release(p7);
	//release(p6);
	//release(p5);
	//release(p4);
	//release(p3);
	//release(p2);
	//release(p1);
	//printf(" ---------------------- \n");

	printf(" -- allocate 1 block of 16K and 2 blocks of 8K \n");
	p1 = allocate(16 * 1024);
	p2 = allocate(8 * 1024);
	p3 = allocate(8 * 1024);
	printf(" -- release allocated blocks \n");
	release(p1);
	release(p2);
	release(p3);
	printf(" ---------------------- \n");

	printf(" -- allocate 2 block of 8K and 4 blocks of 4K \n");
	p1 = allocate(8 * 1024);
	p2 = allocate(8 * 1024);
	p3 = allocate(4 * 1024);
	p4 = allocate(4 * 1024);
	p5 = allocate(4 * 1024);
	p6 = allocate(4 * 1024);
	printf(" -- release allocated blocks \n");
	release(p1);
	release(p2);
	release(p3);
	release(p4);
	release(p5);
	release(p6);
	printf(" ---------------------- \n");

	printf(" -- allocate 8 block of 4K \n");
	p1 = allocate(4 * 1024);
	p2 = allocate(4 * 1024);
	p3 = allocate(4 * 1024);
	p4 = allocate(4 * 1024);

	p5 = allocate(4 * 1024);
	p6 = allocate(4 * 1024);
	p7 = allocate(4 * 1024);
	p8 = allocate(4 * 1024);
	printf(" -- release allocated blocks \n");
	release(p1);
	release(p2);
	release(p3);
	release(p4);
	release(p5);
	release(p6);
	release(p7);
	release(p8);
	printf(" ---------------------- \n");
}

void allocate_blocks(size_t block_size)
{
	printf("allocate blocks of %x bytes: ", block_size);
	size_t block_count = 0;
	while (1) {
		void* p = pool_allocator_alloc(block_size);
		if (!p)
			break;
		block_count++;
	}
	printf(" allocated %x blocks\n", block_count);
}

void test_pool_allocator()
{
	printf("test pool allocator\n");

	allocate_blocks(1024);
	allocate_blocks(512);
	allocate_blocks(256);
	allocate_blocks(128);
	allocate_blocks(64);
	allocate_blocks(32);
	allocate_blocks(16);
	allocate_blocks(8);
}

#define NPAGES 32
void test_allocator()
{
	void* pages[NPAGES] = { NULL };
	for (int i = 0; i < NPAGES; i++) {
		printf("1st: allocate page #%x\n", i);
		pages[i] = allocate(4096);
		if (pages[i] == NULL) {
			printf("1st: could not allocate page #%x!\n", i);
			while (1) ;
		}
	}

	void* p = allocate(4096);
	if (p != NULL) {
		printf("1st: the page must not be allocated !\n");
		while (1) ;
	}

	for (int i = 0; i < NPAGES; i++) {
		release(pages[i]);
		pages[i] = NULL;
	}

	for (int i = 0; i < NPAGES; i++) {
		printf("2nd: allocate page #%x\n", i);
		pages[i] = allocate(4096);
		if (pages[i] == NULL) {
			printf("2nd: could not allocate page #%x!\n", i);
			while (1) ;
		}
	}

	if (p != NULL) {
		printf("2nd: the page must not be allocated !\n");
		while (1) ;
	}
	
	for (int i = 0; i < NPAGES; i++) {
		release(pages[i]);
		pages[i] = NULL;
	}
}

void foo(void *arg) {
	/*
	while (1) {
		uint8_t id = current_thread_id();
		printf("Thread (id: %x)\n", id);
		delay(1000000);
	}
	*/
	
	for (int i = 0; i < 10; i++) {
		uint8_t id = current_thread_id();
		printf("Thread (id: %x) %x\n", id, i);
		delay(1000000);
		// schedule();
	}

	uint8_t id = current_thread_id();
	printf("Thread (id: %x) finished\n", id);
}


void kmain(uint64_t dtb_ptr32)
{
	int rst = 0;
	char cmd[32];

	uart_init();
	load_kernel();
	
	init_exception_table();

	// execute_userprogram();
	core_timer_init();
	enable_interrupts();

	print_board_sn();
	print_board_revision();

	// char* p = malloc(0);
	// printf("heap start adress: %x\n", (intptr_t)p);
	// printf("fdt start address: %x\n", (uint32_t)dtb_ptr32);

	allocator_init();
	// pool_allocator_init();

	// test_allocator();

	/*
	add_timer(&on_timer, NULL, 2);
	add_timer(&on_timer, NULL, 4);
	add_timer(&on_timer, NULL, 3);
	add_timer(&on_timer, NULL, 8);
	*/

	fdt_parse((const uint8_t *)dtb_ptr32, fdt_node_visit);

	for (int i = 0; i < 8; i++) {
		printf("create thread %x\n", i);
		thread_create(&foo, i);
	}

	idle();

	/*
	while (1) {
		uart_putc('>');
		uart_putc(' ');
		
		readline(cmd, 32);
		uart_puts(cmd);
		uart_putc('\r');
		uart_putc('\n');
		
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
			list_files();
		} else if (strncmp(cmd, "cat", 3) == 0) {
			const char *fname = cmd + 3;
			while (*fname && *fname == ' ')
				fname++;
			if (*fname)
				print_file(fname);
		} else if (strcmp(cmd, "exec") == 0) {
			// execute_userprogram();
			uint8_t *stack = malloc(USER_STACK_SIZE);
			if (cpio_exec_file("userprogram", stack) != 0)
				uart_puts("execute userprogram failed\r\n");
		} else {
			uart_puts("unknown command\n");
		}
	}
	*/
}

