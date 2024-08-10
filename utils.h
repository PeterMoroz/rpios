#ifndef __UTILS_H__
#define __UTILS_H__

#include <stdint.h>

extern void branch_to_address(void*);
extern uint64_t get_el();

extern void execute_in_el0(const void*, void*);

#endif

