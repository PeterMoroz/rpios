#ifndef __MEMUTILS_H__
#define __MEMUTILS_H__

#include <stddef.h>

int memcmp(const void *p1, const void *p2, size_t nbytes);
void* memcpy(void *dst, const void *src, size_t nbytes);
void* memset(void *p, char c, size_t n);

#endif
