CROSS=aarch64-linux-gnu-
CC=$(CROSS)gcc
CFLAGS=-Wall -O -ffreestanding

BLINK_OBJS=kernel.o blink.o

all: kernel8.img

kernel8.img: kernel8.elf
	$(CROSS)objcopy -O binary $< $@

kernel8.elf: $(BLINK_OBJS) kernel.ld
	$(CROSS)ld -o $@ $(BLINK_OBJS) -Tkernel.ld

clean:
	rm -f *.o *.img *.elf
