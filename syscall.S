#include "syscall_impl.h"

.globl getpid
getpid:
    mov w8, #SYS_GETPID_NUMBER
    svc #0
    ret

.globl uart_read
uart_read:
    mov w8, #SYS_UART_READ_NUMBER
    svc #0
    ret

.globl uart_write
uart_write:
    mov w8, #SYS_UART_WRITE_NUMBER
    svc #0
    ret

.globl alloc_page
alloc_page:
    mov w8, #SYS_ALLOC_PAGE_NUMBER
    svc #0
    ret

.globl exit
.exit:
    mov w8, #SYS_EXIT_NUMBER
    svc #0
    ret

.globl clone
.clone:
    ret

