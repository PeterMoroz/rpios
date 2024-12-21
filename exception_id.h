#ifndef __EXCEPTION_ID_H__
#define __EXCEPTION_ID_H__

#define SYNCHRONOUS_EL1T 0
#define IRQ_EL1T         1
#define FIQ_EL1T         2
#define SERROR_EL1T      3

#define SYNCHRONOUS_EL1H 4
#define IRQ_EL1H         5
#define FIQ_EL1H         6
#define SERROR_EL1H      7

#define SYNCHRONOUS_EL0_64 8
#define IRQ_EL0_64         9
#define FIQ_EL0_64         10
#define SERROR_EL0_64      11

#define SYNCHRONOUS_EL0_32 12
#define IRQ_EL0_32         13
#define FIQ_EL0_32         14
#define SERROR_EL0_32      15

#define SYNC_EL0_64_ERROR  16
#define SYSCALL_ERROR      17

#endif

