#ifndef __THREAD_H__
#define __THREAD_H__

#include <stdint.h>

/* task flags */
#define TF_KTHREAD 2

typedef void (*thread_function_t)(void *);

void thread_idle();
void schedule();
void thread_sched_tick();
// void thread_create(thread_function_t fn, void *arg);
int thread_create(uint8_t flags, thread_function_t fn, void *arg, void* stack);
int thread_move_to_usermode(thread_function_t fn);

uint8_t current_thread_id();

#endif
