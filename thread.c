#include "thread.h"
#include "allocator.h"
#include "interrupts.h"

// #define _TRACE_PRINT_

#ifdef _TRACE_PRINT_
#include "printf.h"
#endif

#include <stdint.h>

struct cpu_context {
	uint64_t x19;
	uint64_t x20;
	uint64_t x21;
	uint64_t x22;
	uint64_t x23;
	uint64_t x24;
	uint64_t x25;
	uint64_t x26;
	uint64_t x27;
	uint64_t x28;
	uint64_t fp;
	uint64_t sp;
	uint64_t pc;
};

struct task_struct {
	struct cpu_context cpu_context;
	int32_t state;
	int32_t counter;
	int32_t priority;
	int32_t preempt_count;
	uint8_t id;
};


#define TASK_RUNNING  1
#define TASK_FINISHED 2

#define INIT_TASK { { 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0 }, TASK_RUNNING, 0, 1, 0, 0 }

static struct task_struct init_task = INIT_TASK;
static struct task_struct *current = &init_task;

#define NR_TASKS 64
struct task_struct* tasks[NR_TASKS] = { &init_task, NULL, };
static size_t nr_tasks = 1;
static uint8_t next_id = 1;

void cleanup_finished_tasks()
{
	struct task_struct *p;
	size_t i = 1;
	while (i < nr_tasks) {
		p = tasks[i];
		if (p && p->state == TASK_FINISHED) {
			release(p);
			for (size_t j = i + 1; j < nr_tasks; j++) {
				tasks[j - 1] = tasks[j];
			}
			nr_tasks--;
			continue;
		}
		i++;
	}
	/*
	for (size_t i = 1; i < nr_tasks; i++) {
		p = tasks[i];
		if (p && p->state == TASK_FINISHED) {
			release(p);
			tasks[i] = NULL;
		}
	}*/
}

void idle()
{
	while (1) {
		cleanup_finished_tasks();
		schedule();
	}
}

void preempt_disable()
{
	current->preempt_count++;
}

void preempt_enable()
{
	current->preempt_count--;
}

extern void cpu_switch_to(struct task_struct*, struct task_struct*);

void switch_to(struct task_struct *next) {
	if (current == next)
		return ;
	struct task_struct *prev = current;
	current = next;
	cpu_switch_to(prev, next);
}

void _schedule()
{
	preempt_disable();
	int32_t max_cnt;
	size_t next;
	struct task_struct *p;
	while (1) {
		max_cnt = -1;
		next = 0;
		for (size_t i = 0; i < NR_TASKS; i++) {
			p = tasks[i];
			if (p && p->state == TASK_RUNNING && p->counter > max_cnt) {
				max_cnt = p->counter;
				next = i;
			}
		}
		if (max_cnt) {
			break;
		}
		for (size_t i = 0; i < NR_TASKS; i++) {
			p = tasks[i];
			if (p) {
				p->counter = (p->counter >> 1) + p->priority;
			}
		}
	}

#ifdef _TRACE_PRINT_
	printf("schedule: nr_tasks %x switch to %x\n", nr_tasks, next);
#endif

	switch_to(tasks[next]);
	preempt_enable();
}

void thread_sched_tick()
{
#ifdef _TRACE_PRINT_
	printf("thread sched tick - current->counter: %x\n", current->counter);
#endif
	--current->counter;
	if (current->counter > 0 || current->preempt_count > 0) {
		return;
	}
	current->counter = 0;
	enable_irq();
	_schedule();
	disable_irq();
}

void schedule()
{
	current->counter = 0;
	_schedule();
}

void schedule_tail()
{
#ifdef _TRACE_PRINT_
	printf("schedule tail: current->id %x\n", current->id);
#endif
	preempt_enable();
}

void thread_exit()
{
	preempt_disable();
#ifdef _TRACE_PRINT_
	printf("thread exit: current->id %x\n", current->id);
#endif
	current->state = TASK_FINISHED;
	preempt_enable();
	schedule();
}

extern void thread_trampoline();

void thread_create(thread_function_t fn, void *arg)
{
	preempt_disable();
	struct task_struct *p = NULL;
	/* allocate block larger than task_struct, 
	 the remaining space will be used as stack */
	p = allocate(4096);
	if (!p) {
#ifdef _PRINT_TRACE_
		printf("could not allocate memory to store task_struct!\n");
#endif
		return;
	}

	p->priority = current->priority;
	p->state = TASK_RUNNING;
	p->counter = p->priority;
	p->preempt_count = 1;

	p->cpu_context.x19 = (uint64_t)fn;
	p->cpu_context.x20 = (uint64_t)arg;
	p->cpu_context.pc = (uint64_t)thread_trampoline;
	p->cpu_context.sp = (uint64_t)p + 4096;
	size_t idx = nr_tasks++;
	p->id = next_id++;
	tasks[idx] = p;
	preempt_enable();

#ifdef _TRACE_PRINT_
	printf("a new thread created (id %x), pos: %x, total nr: %x\n",
	       p->id, idx, nr_tasks);
#endif
}

uint8_t current_thread_id() { return current->id; }

