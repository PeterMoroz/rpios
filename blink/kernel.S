.section .text.init

.global _start

_start:
1:
    // jump to kmain, which shouldn't return. halt if it does
    bl      kmain
    b       1b
