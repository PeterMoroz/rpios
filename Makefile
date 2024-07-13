CROSS=aarch64-linux-gnu-
CC=$(CROSS)gcc

SRCS = $(wildcard *.c)
OBJS = $(SRCS:.c=.o)
CFLAGS=-Wall -O -ffreestanding -nostdinc -nostdlib -nostartfiles


all: kernel8.img

start.o: start.S
	$(CROSS)gcc $(CFLAGS) -c start.S -o start.o

kernel8.img: kernel8.elf
	$(CROSS)objcopy -O binary $< $@

kernel8.elf: start.o $(OBJS) kernel.ld
	$(CROSS)ld -nostdlib -nostartfiles start.o  $(OBJS) -T kernel.ld -o $@

clean:
	rm -f *.o *.img *.elf
