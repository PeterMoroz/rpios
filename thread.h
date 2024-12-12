#ifndef __THREAD_H__
#define __THREAD_H__

#include <stdint.h>

typedef void (*thread_function_t)(void *);

void idle();
void schedule();
void thread_sched_tick();
void thread_create(thread_function_t fn, void *arg);

uint8_t current_thread_id();

#endif
