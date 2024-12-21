CROSS=aarch64-linux-gnu-
CC=$(CROSS)gcc

SRCS = $(wildcard *.c)
OBJS = $(SRCS:.c=.o)
CFLAGS=-Wall -O -ffreestanding -nostdlib -nostartfiles -g -mno-strict-align


all: kernel8.img

ramdisk:
	cd rootfs && find . | cpio -o -H newc > ../ramdisk && cd ..

core_timer.o: core_timer.S
	$(CROSS)gcc $(CFLAGS) -c core_timer.S -o core_timer.o

exceptions.o: exceptions.S
	$(CROSS)gcc $(CFLAGS) -c exceptions.S -o exceptions.o

interrupts.o: interrupts.S
	$(CROSS)gcc $(CFLAGS) -c interrupts.S -o interrupts.o

start.o: start.S
	$(CROSS)gcc $(CFLAGS) -c start.S -o start.o

syscall.o: syscall.S
	$(CROSS)gcc $(CFLAGS) -c syscall.S -o syscall.o

thread_utils.o: thread_utils.S
	$(CROSS)gcc $(CFLAGS) -c thread_utils.S -o thread_utils.o

utils.o: utils.S
	$(CROSS)gcc $(CFLAGS) -c utils.S -o utils.o

userprogram.o: userprogram.S
	$(CROSS)gcc $(CFLAGS) -c userprogram.S -o userprogram.o

userprogram.elf: userprogram.o
	$(CROSS)ld -nostdlib -nostartfiles userprogram.o -o $@

userprogram: userprogram.elf
	$(CROSS)objcopy -O binary $< $@

kernel8.img: kernel8.elf
	$(CROSS)objcopy -O binary $< $@

kernel8.elf: core_timer.o exceptions.o interrupts.o start.o syscall.o thread_utils.o utils.o $(OBJS) kernel.ld
	$(CROSS)ld -nostdlib -nostartfiles core_timer.o exceptions.o interrupts.o start.o syscall.o thread_utils.o utils.o $(OBJS) -T kernel.ld -o $@
	$(CROSS)objdump -D kernel8.elf > kernel8.lst

runqemu: kernel8.img ramdisk
	qemu-system-aarch64 -M raspi3 -kernel kernel8.img -dtb bcm2710-rpi-3-b-plus.dtb -initrd ramdisk -serial null -serial stdio

dbgqemu: kernel8.img ramdisk
	qemu-system-aarch64 -M raspi3 -kernel kernel8.img -dtb bcm2710-rpi-3-b-plus.dtb -initrd ramdisk -display none -S -s

clean:
	rm -f *.o *.img *.elf *.bin *.lst ramdisk userprogram
