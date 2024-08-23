#define ARM_TIMER_CORE0_IRQ_CTRL 0x40000040


.global core_timer_init
core_timer_init:
  mov x0, 0x1
  msr cntp_ctl_el0, x0
  // mov x0, 0xFFFFFFF
  mrs x0, cntfrq_el0
  msr cntp_tval_el0, x0
  mov x0, 0x2
  ldr x1, =ARM_TIMER_CORE0_IRQ_CTRL
  str x0, [x1]
  ret

.global core_timer_irq_handler
core_timer_irq_handler:
  mrs x0, cntfrq_el0
//  mov x0, x0, lsl 1
  msr cntp_tval_el0, x0
  ret

.global core_timer_get_seconds
core_timer_get_seconds:
  mrs x1, cntpct_el0
  mrs x2, cntfrq_el0
  sdiv x0, x1, x2
  ret

.global core_timer_set_delay
core_timer_set_delay:
  mrs x1, cntfrq_el0
  mul x2, x1, x0
  msr cntp_tval_el0, x2
  ret

