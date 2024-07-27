#include "heap.h"

extern unsigned char __bss_end;
static unsigned char *heap = 0L;

void* malloc(unsigned size)
{
	if (heap == 0L)
		heap = &__bss_end;

	unsigned char *p = heap;
	heap += size;
	return p;
}
