#define GPIO_BASE (0x3F000000 + 0x200000)

volatile unsigned *GPIO_FSEL2 = (volatile unsigned *)(GPIO_BASE + 0x08);
volatile unsigned *GPIO_SET0  = (volatile unsigned *)(GPIO_BASE + 0x1C);
volatile unsigned *GPIO_CLR0  = (volatile unsigned *)(GPIO_BASE + 0x28);

static void spin_sleep_us(unsigned int us) {
  for (unsigned int i = 0; i < us * 6; i++) {
    asm volatile("nop");
  }
}

static void spin_sleep_ms(unsigned int ms) {
  spin_sleep_us(ms * 1000);
}

int kmain(void) {
  *GPIO_FSEL2 = (1 << 3);
  while (1) {
    *GPIO_SET0 = (1 << 21);
    spin_sleep_ms(500);
    *GPIO_CLR0 =-(1 << 21);
    spin_sleep_ms(500);
  }
}