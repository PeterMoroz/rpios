#include "pool_allocator.h"
#include "allocator.h"
#include "memutils.h"

#include <stdint.h>

struct pool
{
	void* head;
	void* buffer;
};

/* 
 * pools' sizes: 8, 16, 32, 64, 128, 256, 512, 1024 
 */

#define NPOOLS 8
#define PAGE_SIZE 4096

static struct pool pools[NPOOLS];

static void pool_init(struct pool* pool, uint8_t* buffer, size_t size, size_t bit_shift)
{
	// the chunk size represented as bit shift
	// size of pool must be power of 2
	const size_t chunk_count = size >> bit_shift;
	const size_t chunk_size = 1 << bit_shift;
	size_t offset = 0;
	uint8_t* chunk = buffer;
	for (size_t i = 0; i < chunk_count - 1; i++) {
		chunk = buffer + offset;
		*((uint8_t **)chunk) = chunk + chunk_size;
		offset += chunk_size;
	}

	chunk = buffer + offset;
	*((uint8_t **)chunk) = NULL;

	pool->head = buffer;
	pool->buffer = buffer;
}

static void* pool_alloc(struct pool* pool)
{
	if (!pool->head)
		return NULL;

	uint8_t* p = pool->head;
	pool->head = (*((uint8_t **)(pool->head)));
	return p;
}

static void pool_free(struct pool* pool, void* p)
{
	if (!p)
		return;
	*((uint8_t **)p) = pool->head;
	pool->head = (uint8_t *)p;
}

void pool_allocator_init()
{
	memset(&pools, 0, sizeof(pools));

	size_t bit_shift = 3;
	for (size_t i = 0; i < NPOOLS; i++) {
		void* buffer = allocate(PAGE_SIZE);
		pool_init(&pools[i], buffer, PAGE_SIZE, bit_shift);
		bit_shift++;
	}
}

void* pool_allocator_alloc(size_t size)
{
	size_t i = 0;
	size_t bit_shift = 3;
	for (; i < NPOOLS; i++) {
		const size_t chunk_size = 1 << bit_shift;
		if (size <= chunk_size)
			break;
		bit_shift++;
	}

	// requested size is too large
	if (i == NPOOLS)
		return 0L;

	return pool_alloc(&pools[i]);
}

void pool_allocator_free(void* p)
{
	// find the pool to which block belongs
	const size_t prefix_mask = ~(PAGE_SIZE - 1);
	const uintptr_t prefix = (uintptr_t)p & prefix_mask;
	size_t i = 0;
	for (; i < NPOOLS; i++) {
		if (((uintptr_t)(pools[i].buffer) & prefix_mask) == prefix)
			break;
	}
	
	// something wrong !
	if (i == NPOOLS)
		return;

	pool_free(&pools[i], p);
}

