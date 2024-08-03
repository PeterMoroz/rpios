#ifndef __CPIO_H__
#define __CPIO_H__

#include <stdint.h>
#include <stddef.h>

typedef void(*putchar_cb_t)(char);

void cpio_read_catalog(putchar_cb_t putchar_cb);
int cpio_file_size(const char *fname);
int cpio_read_file(const char *fname, char *buffer, size_t buffer_size);

void cpio_set_initrd_start(uint64_t addr);

#endif
