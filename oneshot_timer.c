#include "oneshot_timer.h"
#include "core_timer.h"
#include "uart.h"

typedef struct oneshot_timer
{
	timer_callback callback;
	void* data;
	uint32_t start;
	uint32_t delay;
} oneshot_timer;

#define MAX_NUM_OF_TIMERS 255

static uint8_t num_of_timers = 0;

static oneshot_timer timers[MAX_NUM_OF_TIMERS];

void schedule_next_event()
{
	if (num_of_timers == 0) {
		core_timer_set_delay(1);
		return;
	}
	
	uint8_t nearest_time = timers[0].start + timers[0].delay;
	for (uint8_t i = 1; i < num_of_timers; i++) {
		if (timers[i].start + timers[i].delay < nearest_time) {
			nearest_time = timers[i].start + timers[i].delay;
		}
	}

	uint32_t delay = nearest_time - core_timer_get_seconds();
	core_timer_set_delay(delay);
}

void add_timer(timer_callback callback, void* data, uint32_t delay)
{
	if (num_of_timers < MAX_NUM_OF_TIMERS)
	{
		uint32_t start = core_timer_get_seconds();
		timers[num_of_timers++] = (oneshot_timer){ .callback = callback, .data = data, .start = start, .delay = delay };
		schedule_next_event();
	}
}

void remove_timer(uint8_t pos)
{
	if (num_of_timers == 0)
		return;
	if (pos == num_of_timers - 1) {
		num_of_timers--;
		return;
	}

	uint8_t i = pos, j = i + 1;
	for (; j < num_of_timers; i++, j++) {
		timers[i] = timers[j];
	}
	num_of_timers--;
}

void print_timer_event(oneshot_timer* timer)
{
	uart_puts("timer event: ");
	uart_puts(" start = ");
	uart_put_uint32_hex(timer->start);
	uart_puts(" delay = ");
	uart_put_uint32_hex(timer->delay);
	uart_putc('\r');
	uart_putc('\n');
}

void oneshot_timer_tick()
{
	uint32_t now = core_timer_get_seconds();
	for (uint8_t i = 0; i < num_of_timers; i++) {
		oneshot_timer* timer = &timers[i];
		if (timer->start + timer->delay == now) {
			timer->callback(timer->data);
			// print_timer_event(timer);
			remove_timer(i);
			continue;
		}
	}
	schedule_next_event();
}

