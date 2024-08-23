#ifndef __CORE_TIMER_H__
#define __CORE_TIMER_H__

#include <stdint.h>

void core_timer_init();
void core_timer_irq_handler();
uint32_t core_timer_get_seconds();

void core_timer_set_delay(uint32_t seconds);

#endif
