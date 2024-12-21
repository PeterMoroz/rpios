#include "allocator.h"
#include "thread.h"
#include "uart.h"

#include <stddef.h>

int sys_getpid() {
	return current_thread_id();
}

size_t sys_uart_read(char* buff, size_t size) {
	size_t n = 0;
	for (; n < size; n++) {
		buff[n] = uart_getc();
	}
	return n;
}

size_t sys_uart_write(const char* buff, size_t size) {
	size_t n = 0;
	for (; n < size; n++) {
		uart_putc(buff[n]);
	}
	return n;
}

void* sys_alloc_page() {
	void *p = allocate(4096);
	return p;
}

int sys_clone() {
	return -1;
}

void sys_exit(int code) {
}

void* sys_call_table[] = { sys_getpid, sys_uart_read, sys_uart_write, 
			sys_alloc_page, sys_clone, sys_exit };

