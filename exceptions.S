#include "exception_id.h"
#include "execution_context.inc"


.macro exception_handler_stub id
// TO DO: disable interrupts
save_context
// TO DO: enable interrupts
mov x0, #\id
mrs x1, spsr_el1
mrs x2, esr_el1
mrs x3, elr_el1
bl print_exception_info
// TO DO: disable interrupts
load_context
// TO TO: enable interrupts
eret
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
    exception_handler_stub SYNCHRONOUS_EL1T

irq_el1t:
    exception_handler_stub IRQ_EL1T

fiq_el1t:
    exception_handler_stub FIQ_EL1T

serror_el1t:
    exception_handler_stub SERROR_EL1T

synchronous_el1h:
    exception_handler_stub SYNCHRONOUS_EL1H

irq_el1h:
    exception_handler_stub IRQ_EL1H

fiq_el1h:
    exception_handler_stub FIQ_EL1H

serror_el1h:
    exception_handler_stub SERROR_EL1H

synchronous_el0_64:
//    exception_handler_stub SYNCHRONOUS_EL0_64
    mrs x25, esr_el1
    lsr x24, x25, #26
    cmp x24, #0x15
    b.eq svc_el0_64
    load_context
    eret

irq_el0_64:
    exception_handler_stub IRQ_EL0_64

fiq_el0_64:
    exception_handler_stub FIQ_EL0_64

serror_el0_64:
    exception_handler_stub SERROR_EL0_64

synchronous_el0_32:
    exception_handler_stub SYNCHRONOUS_EL0_32

irq_el0_32:
    exception_handler_stub IRQ_EL0_32

fiq_el0_32:
    exception_handler_stub FIQ_EL0_32

serror_el0_32:
    exception_handler_stub SERROR_EL0_32


svc_el0_64:
    load_context
    ret

.globl init_exception_table
init_exception_table:
    adr x0, vectors
    msr vbar_el1, x0
    ret

