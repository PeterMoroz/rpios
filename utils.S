#include "execution_context.inc"

.global branch_to_address
branch_to_address:
    br x0

.global get_el
get_el:
    mrs x0, CurrentEL
    lsr x0, x0, #2
    ret

.global execute_in_el0
execute_in_el0:
    save_context
    mov x2, 0x3c0
    msr spsr_el1, x2
    msr sp_el0, x1
    msr elr_el1, x0
    eret
