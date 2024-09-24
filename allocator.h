#ifndef __ALLOCATOR_H__
#define __ALLOCATOR_H__

#include <stddef.h>

void allocator_init();
void* allocate(size_t size);
void release(void* p);

#endif
