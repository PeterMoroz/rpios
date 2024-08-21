#define INTERRUPT_ENABLE_1 0x3F00B210
#define BASE_INTERRUPT_ENABLE 0x3F00B218

.global enable_interrupts
enable_interrupts:
  // mov x0, 0x2
  ldr x0, =((1 << 29) | (1 << 2))
  ldr x1, =INTERRUPT_ENABLE_1
  str x0, [x1]
  mov x0, 0x1
  ldr x1, =BASE_INTERRUPT_ENABLE
  str x0, [x1]
  msr daifclr, #0x2
  ret

.global enable_irq
enable_irq:
  msr daifclr, #0x2
  ret

.global disable_irq
disable_irq:
  msr daifset, #0x2
  ret

