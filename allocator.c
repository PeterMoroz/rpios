#include "allocator.h"
#include "log2.h"

#include <stdint.h>

#define _TRACE_PRINT_

#ifdef _TRACE_PRINT_
#include "printf.h"
#endif



#define ALLOCATED 0x1000
#define AVAILABLE 0x2000

#define PAGE_BIT_SHIFT 12
#define PAGE_SIZE (1 << PAGE_BIT_SHIFT)

#define MAX_ORDER 3
#define ALLOCABLE_SIZE (32 * 1024)

#define BASE_ADDRESS 0x10000000

#define NITEMS (ALLOCABLE_SIZE / PAGE_SIZE)

#define ABS(x) ((x) < 0 ? -(x) : (x))

static uint16_t frame_array[NITEMS];

void print_array();


void allocator_init()
{
	frame_array[0] = log2(NITEMS);  // max order
	for (int i = 1; i < NITEMS; i++) {
		frame_array[i] = AVAILABLE;
	}
	
#ifdef _TRACE_PRINT_
	printf("init allocator\t NITEMS: %x  base: %x \n", NITEMS, BASE_ADDRESS);
	print_array();	
#endif	
}

static void* allocate_frames(size_t size)
{
#ifdef _TRACE_PRINT_
	printf("allocate block of %x bytes\n", size);
#endif
	
	// find the minimal suitable block
	int idx = -1;
	size_t best_suited_size = 0xFFFFFFFF;
	for (int i = 0; i < NITEMS; i++) {
		if (frame_array[i] >= ALLOCATED)
			continue;
		size_t block_size = (1 << frame_array[i]) << PAGE_BIT_SHIFT;
		if (block_size >= size) {
			if (idx == -1 || block_size < best_suited_size) {
				idx = i;
				best_suited_size = block_size;
			}
		}
	}

#ifdef _TRACE_PRINT_
	int buddy_idx = idx ^ (1 << frame_array[idx]);
	printf("index: %x buddy_index: %x\n", idx, buddy_idx);
#endif

	uint16_t order = frame_array[idx];
	size_t block_size = (1 << order) << PAGE_BIT_SHIFT;
	while (block_size >= 2 * size) {
		frame_array[idx] -= 1;
		order = frame_array[idx];
		buddy_idx = idx ^ (1 << order);
		frame_array[buddy_idx] = order;
		block_size = (1 << order) << PAGE_BIT_SHIFT;
	}

	const int n = ABS(idx - buddy_idx);
	frame_array[idx] = ALLOCATED | order;
	for (int i = idx + 1; i < (idx + n); i++)
		frame_array[i] = ALLOCATED;

#ifdef _TRACE_PRINT_
	print_array();
#endif

	return ((char *)BASE_ADDRESS + (idx << PAGE_BIT_SHIFT));
}

void* allocate(size_t size)
{
	return allocate_frames(size);
}

static void merge(int lpos, int rpos)
{
	if (lpos == rpos)
		return;

	const int distance = rpos - lpos;
	int mid = lpos + (distance >> 1);
	if (distance > 1) {
		merge(lpos, mid);
		merge(mid, rpos);
	}
	
	const int order = log2(distance);
	if (frame_array[lpos] == frame_array[mid] && (order - 1) == frame_array[lpos]) {
		frame_array[lpos] = order;
		for (int i = lpos + 1; i < rpos; i++)
			frame_array[i] = AVAILABLE;
	}
}

void release(void* p)
{
  int idx = ((intptr_t)((char *)p - BASE_ADDRESS)) >> PAGE_BIT_SHIFT;
#ifdef _TRACE_PRINT_
	printf("release block at %x\n", idx);
#endif

	// block of order 0, just mark it as free
	if (frame_array[idx] == ALLOCATED) {
		frame_array[idx] = 0;
	}
	else {
		uint16_t order = frame_array[idx] & (~ALLOCATED);		
		int buddy_idx = idx ^ (1 << order);
		const int n = ABS(idx - buddy_idx);
		frame_array[idx] = order;
		for (int i = idx + 1; i < (idx + n); i++)
			frame_array[i] = AVAILABLE;
	}

	// merge buddies
	merge(0, NITEMS);

#ifdef _TRACE_PRINT_
	print_array();
#endif
}

#ifdef _TRACE_PRINT_
void print_array()
{
	printf("frames array: ");
	for (int i = 0; i < NITEMS; i++) {
	  printf("%x ", frame_array[i]);
	}
	printf("\n");
}
#endif

