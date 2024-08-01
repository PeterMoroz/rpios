CROSS=aarch64-linux-gnu-
CC=$(CROSS)gcc

SRCS = $(wildcard *.c)
OBJS = $(SRCS:.c=.o)
CFLAGS=-Wall -O -ffreestanding -nostdlib -nostartfiles -g -mno-strict-align


all: kernel8.img

ramdisk:
	cd rootfs && find . | cpio -o -H newc > ../ramdisk && cd ..

rd.o: ramdisk
	$(CROSS)ld -r -b binary -o rd.o ramdisk

start.o: start.S
	$(CROSS)gcc $(CFLAGS) -c start.S -o start.o

utils.o: utils.S
	$(CROSS)gcc $(CFLAGS) -c utils.S -o utils.o

kernel8.img: kernel8.elf
	$(CROSS)objcopy -O binary $< $@

kernel8.elf: start.o utils.o $(OBJS) rd.o kernel.ld
	$(CROSS)ld -nostdlib -nostartfiles start.o rd.o utils.o $(OBJS) -T kernel.ld -o $@

runqemu: kernel8.img
	qemu-system-aarch64 -M raspi3 -kernel kernel8.img -serial null -serial stdio

dbgqemu: kernel8.img
	qemu-system-aarch64 -M raspi3 -kernel kernel8.img -display none -S -s

clean:
	rm -f *.o *.img *.elf ramdisk
