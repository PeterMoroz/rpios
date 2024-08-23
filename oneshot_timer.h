#ifndef __ONESHOT_TIMER_H__
#define __ONESHOT_TIMER_H__

#include <stdint.h>

typedef void (*timer_callback)(void*);

void add_timer(timer_callback callback, void* data, uint32_t delay);
void oneshot_timer_tick();

#endif
