#include "gpio.h"


unsigned mbox_compose(unsigned char ch, unsigned data)
{
	unsigned mail  = data << 4;
	mail |= ch & 0xF;
	return mail;
}


#define VIDEOCORE_MBOX (MMIO_BASE + 0x0000B880)
#define MBOX0_READ    ((volatile unsigned int*)(VIDEOCORE_MBOX+0x0))
#define MBOX0_STATUS  ((volatile unsigned int*)(VIDEOCORE_MBOX+0x18))
#define MBOX1_WRITE   ((volatile unsigned int*)(VIDEOCORE_MBOX+0x20))
#define MBOX1_STATUS  ((volatile unsigned int*)(VIDEOCORE_MBOX+0x38))

#define MBOX_FULL  0x80000000
#define MBOX_EMPTY 0x40000000

void mbox_put(unsigned mail)
{
	do { asm volatile("nop"); } while(*MBOX1_STATUS & MBOX_FULL);
	*MBOX1_WRITE = mail;
}

unsigned mbox_get(void)
{
	unsigned mail;
	do { asm volatile("nop"); } while (*MBOX0_STATUS & MBOX_EMPTY);
	mail = *MBOX0_READ;
	return mail;
}

