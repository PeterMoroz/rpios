#ifndef __CPIO_H__
#define __CPIO_H__

typedef void(*putchar_cb_t)(unsigned char);

void cpio_read_catalog(putchar_cb_t putchar_cb);
int cpio_file_size(const char *fname);
int cpio_read_file(const char *fname, char *buffer, int buffer_size);

#endif
