#include "memutils.h"

int memcmp(const void *p1, const void *p2, size_t nbytes)
{
	const unsigned char *s1 = p1, *s2 = p2;
	for (size_t i = 0; i < nbytes; i++) {
		int v = s1[i] - s2[i];
		if (v)
			return v;
	}
	return 0;
}

void* memcpy(void *dst, const void *src, size_t nbytes) {
	unsigned char *d = dst;
	const unsigned char *s = src;
	for (size_t i = 0; i < nbytes; i++)
		d[i] = s[i];
	return dst;
}

void* memset(void *p, char c, size_t n) {
	char *b = p, *e = p + n;
	while (b < e)
		*b++ = c;
	return p;
}

