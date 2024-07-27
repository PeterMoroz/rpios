#ifndef __MEMUTILS_H__
#define __MEMUTILS_H__

int memcmp(const void *p1, const void *p2, int nbytes);
void* memcpy(void *dst, const void *src, int nbytes);
void* memset(void *p, char c, int n);

#endif
