#include "thread.h"
#include "allocator.h"
#include "interrupts.h"
#include "memutils.h"

#define _TRACE_PRINT_

#ifdef _TRACE_PRINT_
#include "kprintf.h"
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
	int32_t counter;
	int32_t priority;
	int32_t preempt_count;
	void *stack;
	uint8_t flags;
	uint8_t state;
	uint8_t id;
};

struct pt_regs {
	uint64_t regs[31];
	uint64_t sp;
	uint64_t pc;
	uint64_t pstate;
};

static struct pt_regs* task_pt_regs(struct task_struct *task);

/* PSR mode */
#define PSR_MODE_EL0t 0x00000000
#define PSR_MODE_EL1t 0x00000004
#define PSR_MODE_EL1h 0x00000005
#define PSR_MODE_EL2t 0x00000008
#define PSR_MODE_EL2h 0x00000009
#define PSR_MODE_EL3t 0x0000000c
#define PSR_MODE_EL3h 0x0000000d


/* task state(s) */
#define TS_RUNNING  1
#define TS_FINISHED 2

#define INIT_TASK { { 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0 }, 0, 1, 0, NULL, TF_KTHREAD, TS_RUNNING, 0 }

static struct task_struct init_task = INIT_TASK;
static struct task_struct *current = &init_task;


#define PAGE_SIZE (1 << 12)        // TO DO: put macrodefinitions in one file
#define TASK_FRAME_SIZE (PAGE_SIZE)

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
		if (p && p->state == TS_FINISHED) {
			release(p);
			for (size_t j = i + 1; j < nr_tasks; j++) {
				tasks[j - 1] = tasks[j];
			}
			nr_tasks--;
			continue;
		}
		i++;
	}
}

void thread_idle()
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
			if (p && p->state == TS_RUNNING && p->counter > max_cnt) {
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
	kprintf("schedule: nr_tasks %x switch to %x\n", nr_tasks, next);
#endif

	switch_to(tasks[next]);
	preempt_enable();
}

void thread_sched_tick()
{
#ifdef _TRACE_PRINT_
	kprintf("thread sched tick - current->counter: %x\n", current->counter);
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
	kprintf("schedule tail: current->id %x\n", current->id);
#endif
	preempt_enable();
}

void thread_exit()
{
	preempt_disable();
#ifdef _TRACE_PRINT_
	kprintf("thread exit: current->id %x\n", current->id);
#endif
	current->state = TS_FINISHED;
	preempt_enable();
	schedule();
}

extern void thread_trampoline();

/*
void thread_create(thread_function_t fn, void *arg)
{
	preempt_disable();
	struct task_struct *p = NULL;
	// allocate block larger than task_struct, 
	// the remaining space will be used as stack
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
*/

int thread_create(uint8_t flags, thread_function_t fn, void *arg, void *stack)
{
	preempt_disable();
	struct task_struct *p = NULL;
	/* allocate block larger than task_struct, 
	 the remaining space will be used as stack of kernel thread */
	p = allocate(TASK_FRAME_SIZE);
	if (!p) {
#ifdef _PRINT_TRACE_
		kprintf("could not allocate memory to store task_struct!\n");
#endif
		return -1;
	}

	struct pt_regs *childregs = task_pt_regs(p);
	memset(childregs, 0, sizeof(struct pt_regs));
	memset(&p->cpu_context, 0, sizeof(struct cpu_context));

	if (flags == TF_KTHREAD) {
		p->cpu_context.x19 = (uint64_t)fn;
		p->cpu_context.x20 = (uint64_t)arg;
	} else {
		*childregs = *task_pt_regs(current);
		memcpy(childregs, task_pt_regs(current), sizeof(struct pt_regs));
		childregs->regs[0] = 0;
		childregs->sp = (uint64_t)stack + PAGE_SIZE; // assume that exactly one page allocated fot user thread stack frame
		p->stack = stack;
	}

	p->flags = flags;
	p->priority = current->priority;
	p->state = TS_RUNNING;
	p->counter = p->priority;
	p->preempt_count = 1;

	p->cpu_context.pc = (uint64_t)thread_trampoline;
	p->cpu_context.sp = (uint64_t)childregs;
	size_t idx = nr_tasks++;
	p->id = next_id++;
	tasks[idx] = p;
	preempt_enable();

#ifdef _TRACE_PRINT_
	kprintf("a new thread created (id %x), id %x, total nr: %x\n",
	       p->id, idx, nr_tasks);
#endif
	return idx;
}

int thread_move_to_usermode(thread_function_t fn)
{
	struct pt_regs *regs = task_pt_regs(current);
	memset(regs, 0, sizeof(struct pt_regs));
	regs->pc = (uint64_t)fn;
	regs->pstate = PSR_MODE_EL0t;

	void* stack = allocate(PAGE_SIZE);
	if (!stack) {
		return -1;
	}

	regs->sp = (uint64_t)stack + PAGE_SIZE;
	current->stack = stack;
	return 0;
}

uint8_t current_thread_id() { return current->id; }

static struct pt_regs* task_pt_regs(struct task_struct *task) 
{
	uint64_t p = (uint64_t)task + TASK_FRAME_SIZE - sizeof(struct pt_regs);
	return (struct pt_regs *)p;
}

