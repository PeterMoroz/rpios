.section ".text.boot"
.global _start

_start:
  mrs x1, mpidr_el1
  and x1, x1, #3
  cbz x1, 2f
1:  // cpu id > 0, halt
  wfe
  b 1b
2: // cpu id == 0

  ldr x1, =_start

  mov x2, (1 << 31)
  msr hcr_el2, x2
  mov x2, 0x3c5
  msr spsr_el2, x2
  # msr sp_el1, x1
  ldr x2, =el1_entry
  msr elr_el2, x2
  eret


el1_entry:
  mov sp, x1

  ldr x1, =__bss_start
  ldr w2, =__bss_size
3:
  cbz w2, 4f
  str xzr, [x1], #8
  sub w2, w2, #1
  cbnz w2, 3b

4:
  bl kmain
  b 1b
