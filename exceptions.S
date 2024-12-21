#include "exception_id.h"
// is it needed still ?
#include "execution_context.inc"
#include "irq.inc"

#include "kernel_context.inc"
#include "syscall_impl.h"

// exception handler stub is not used anymore
.macro exception_handler_stub id
disable_irq
save_context
# enable_irq
mov x0, #\id
mrs x1, spsr_el1
mrs x2, esr_el1
mrs x3, elr_el1
bl print_exception_info
# disable_irq
load_context
enable_irq
eret
.endm


.macro handle_invalid_entry el, id
kernel_entry \el
mov x0, #\id
mrs x1, spsr_el1
mrs x2, esr_el1
mrs x3, elr_el1
mov x4, x24
mov x5, x25
// debug version which prints two extra registers (x24, x25)
bl print_exception_info2
b err_hang
.endm

.macro ventry label
.align 7
b      \label
.endm

.align 11
.globl vectors
vectors:
	ventry synchronous_el1t
	ventry irq_el1t
	ventry fiq_el1t
	ventry serror_el1t

	ventry synchronous_el1h
	ventry irq_el1h
	ventry fiq_el1h
	ventry serror_el1h

	ventry synchronous_el0_64
	ventry irq_el0_64
	ventry fiq_el0_64
	ventry serror_el0_64

	ventry synchronous_el0_32
	ventry irq_el0_32
	ventry fiq_el0_32
	ventry serror_el0_32


synchronous_el1t:
    handle_invalid_entry 1, SYNCHRONOUS_EL1T

irq_el1t:
    handle_invalid_entry 1, IRQ_EL1T

fiq_el1t:
    handle_invalid_entry 1, FIQ_EL1T

serror_el1t:
    handle_invalid_entry 1, SERROR_EL1T

synchronous_el1h:
    handle_invalid_entry 1, SYNCHRONOUS_EL1H

irq_el1h:
    kernel_entry 1
    bl handle_irq_el1_64
    kernel_exit 1

fiq_el1h:
    handle_invalid_entry 1, FIQ_EL1H

serror_el1h:
    handle_invalid_entry 1, SERROR_EL1H

synchronous_el0_64:
    // handle_invalid_entry 0, SYNCHRONOUS_EL0_64
    kernel_entry 0
    mrs x25, esr_el1
    lsr x24, x25, #26
    cmp x24, #0x15
    b.eq svc_el0_64
    handle_invalid_entry 0, SYNC_EL0_64_ERROR

irq_el0_64:
    kernel_entry 0
    bl handle_irq_el0_64
    kernel_exit 0

fiq_el0_64:
    handle_invalid_entry 0, FIQ_EL0_64

serror_el0_64:
    handle_invalid_entry 0, SERROR_EL0_64

synchronous_el0_32:
    handle_invalid_entry 0, SYNCHRONOUS_EL0_32

irq_el0_32:
    handle_invalid_entry 0, IRQ_EL0_32

fiq_el0_32:
    handle_invalid_entry 0, FIQ_EL0_32

serror_el0_32:
    handle_invalid_entry 0, SERROR_EL0_32


svc_el0_64:
    adr x27, sys_call_table
    uxtw x26, w8
    mov x25, #__NR_syscalls
    bl enable_irq
    cmp x26, x25
    b.hs ni_sys
    ldr x16, [x27, x26, lsl #3]
    blr x16
    b ret_from_syscall
ni_sys:
    handle_invalid_entry 0, SYSCALL_ERROR
ret_from_syscall:
    bl disable_irq
    str x0, [sp, #0]
    kernel_exit 0

.globl init_exception_table
init_exception_table:
    adr x0, vectors
    msr vbar_el1, x0
    ret

.globl err_hang
err_hang: b err_hang
