CROSS=aarch64-linux-gnu-
CC=$(CROSS)gcc
CFLAGS=-Wall -O -g

KERNEL_OBJS=kernel.o

all: kernel8.img

kernel8.img: kernel8.elf
	$(CROSS)objcopy -O binary $< $@

kernel8.elf: $(KERNEL_OBJS) kernel.ld
	$(CROSS)ld -o $@ $(KERNEL_OBJS) -Tkernel.ld

clean:
	rm -f $(KERNEL_OBJS) *.img *.elf
