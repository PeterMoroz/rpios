#ifndef __RING_BUFFER_H__
#define __RING_BUFFER_H__

// The buffer size have to be power of two!
#define RING_BUFFER_SZ 256

#include <stdint.h>

struct ring_buffer_t
{
	char buffer[RING_BUFFER_SZ];
	uint16_t head;
	uint16_t tail;
};

typedef struct ring_buffer_t ring_buffer_t;

void ring_buffer_init(ring_buffer_t* rb);
void ring_buffer_put(ring_buffer_t* rb, char c);
uint8_t ring_buffer_get(ring_buffer_t* rb, char* c);
uint8_t ring_buffer_is_empty(ring_buffer_t* rb);
uint8_t ring_buffer_is_full(ring_buffer_t* rb);
uint16_t ring_buffer_num_items(ring_buffer_t* rb);

#endif

