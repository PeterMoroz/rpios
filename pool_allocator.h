#ifndef __POOL_ALLOCATOR_H__
#define __POOL_ALLOCATOR_H__

#include <stddef.h>

void pool_allocator_init();
void* pool_allocator_alloc(size_t size);
void pool_allocator_free(void* p);

#endif
