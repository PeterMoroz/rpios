#include "heap.h"

extern unsigned char _end;
static unsigned char *heap = 0L;

void* malloc(size_t size)
{
	if (heap == 0L)
		heap = &_end;

	unsigned char *p = heap;
	heap += size;
	return p;
}
