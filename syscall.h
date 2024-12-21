#ifndef __SYSCALL_H__
#define __SYSCALL_H__

#include <stddef.h>

int getpid();
size_t uart_read(char* buff, size_t size);
size_t uart_write(const char* buff, size_t size);
void* alloc_page();
int clone();
void exit(int code);

#endif
