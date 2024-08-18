.macro enable_irq
msr daifclr, #2
.endm

.macro disable_irq
msr daifset, #2
.endm
