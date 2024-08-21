#include "ring_buffer.h"
#include "memutils.h"

#define BUFFER_MASK (RING_BUFFER_SZ - 1)

void ring_buffer_init(ring_buffer_t* rb)
{
	memset(rb, 0, sizeof(ring_buffer_t));
}

void ring_buffer_put(ring_buffer_t* rb, char c)
{
	if (ring_buffer_is_full(rb)) {
		rb->tail = ((rb->tail + 1) & BUFFER_MASK);
	}
	
	rb->buffer[rb->head] = c;
	rb->head = ((rb->head + 1) & BUFFER_MASK);
}

uint8_t ring_buffer_get(ring_buffer_t* rb, char* c)
{
	if (ring_buffer_is_empty(rb)) {
		return 0;
	}

	*c = rb->buffer[rb->tail];
	rb->tail = ((rb->tail + 1) & BUFFER_MASK);
	return 1;
}

uint8_t ring_buffer_is_empty(ring_buffer_t* rb)
{
	return rb->head == rb->tail ? 1 : 0;
}

uint8_t ring_buffer_is_full(ring_buffer_t* rb)
{
	return ((rb->head - rb->tail) & BUFFER_MASK) == BUFFER_MASK ? 1 : 0;
}

uint16_t ring_buffer_num_items(ring_buffer_t* rb)
{
	return (rb->head - rb->tail) & BUFFER_MASK;
}

