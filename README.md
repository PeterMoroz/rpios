# RPI OS
Experimentation with bare-metal RPI and learning of writing the OS kernel. 
Based on the https://oscapstone.github.io/labs/overview.html 

## Lab 0
The main goal of this lab are:
* setup build (and debug) environment
* make a minimal kernel skeleton and run it on emulator (QEMU)
* prepare SD card to boot RPI

The compiling of binary files to run on the machine with architecture different from that one at which these files are compiled is called "cross compiling".

The minimal set of packages and tools for cross compiling for 64-bit ARM on host machine (Ubuntu x86_64 in my case) are: GCC and binutils. 

```
$ sudo apt install gcc-aarch64-linux-gnu binutils-aarch64-linux-gnu
```

In many cases it would be useful to run the freshlybaked kernel on emulator rather than on real hardware. QEMU is a versatile and powerful tool that can simulate different hardware platforms. QEMU could be built from sources or installed from repositories.

Install cross-platform GDB
```
$ sudo apt install gdb-multiarch
```

#### A kernel skeleton
A minimal kernel skeleton is written in assembler language and does nothing except that roll over an infinite loop.

```
_start:
  wfe
  b _start
```

#### Linker script
The linker script lets the linker know where to put code and data in memory.

#### Building the kernel image
* compile  the source code and produce an object file
`$ aarch64-linux-gnu-gcc -c -g kernel.S`
* produce an ELF file from the object file
`$ aarch64-linux-gnu-ld -T kernel.ld -o kernel8.elf kernel.o`
* strip the ELF header and generate a binary image
`$ aarch64-linux-gnu-objcopy -O binary kernel8.elf kernel8.img`

#### Debug on QEMU
* launch the kernel
`$ qemu-system-aarch64 -M raspi3 -kernel kernel8.img -display none -S -s`
* connect with GDB in another terminal
`$ gdb-multiarch ./kernel8.elf -ex 'target remote localhost:1234' -ex 'break *0x80000' -ex 'continue'`


#### Prepare SD card

###### The boot sequence explanation
When the power is on, the CPU is not active yet, only the GPU (VideoCore) is. The boot sequence is a multi-stage sequence. The GPU starts by executing the *first stage* bootloader stored in ROM which, among other thins, will attempt to read the SD card and load the file `bootcode.bin` which contains the *second stage* bootloader. The code in the file `bootcode.bin` will load the main (GPU) firmware named `start.elf` as well as another file names `fixup.dat`, both located on the SD card. The later is used to configure the SDRAM depending on the hardware. The `start.elf` file contains the code to display the rainbow splash when we have a screen connected to a RPI but its main task is to boot the CPUs and load the kernel code.
It is possible to get diagnostic information of boot process on UART0. To check if UART is supported in current firmware.
`$ strings bootcode.bin | grep BOOT_UART`
To enable UART from `bootcode.bin` 
`$ sed -i -e "s/BOOT_UART=0/BOOT_UART=1/" bootcode.bin`


###### 1. Partition the disk
* erase the device first
`$ sudo dd if=/dev/zero of=/dev/sdd bs=4K && sync`

* make a new partition on the device
`$ sudo fdisk /dev/sdd`

The first thing we have to do is create a new partition table, An MBR formatted  device uses a DOS partition table.
```
Command (m for help): o
Created a new DOS disklabel with disk identifier 0xf78fd9cf.
```

Create a new partition.
```
Command (m for help): n
Partition type
   p   primary (0 primary, 0 extended, 4 free)
   e   extended (container for logical partitions)
Select (default p): p
Partition number (1-4, default 1): 
First sector (2048-15564799, default 2048): 8192
```
I didn't use the default value. 8192 is what's used in the actual Raspbian images and seems to be a common practice with SD cards.

```
Last sector, +/-sectors or +/-size{K,M,G,T,P} (8192-15564799, default 15564799): 

Created a new partition 1 of type 'Linux' and of size 7,4 GiB.
```

By default the type of the parition is "Linux"=EXT3, we have to change it
```
Command (m for help): t
Selected partition 1
Hex code (type L to list all codes): c
Changed type of partition 'Linux' to 'W95 FAT32 (LBA)'.
```

And finally, write the changes are made and exit
```
Command (m for help): w
The partition table has been altered.
Calling ioctl() to re-read partition table.
Syncing disks.
```

###### 2. Create filesystem
To format a partition to FAT32:
`$ sudo mkfs.vfat /dev/sdd1 -n rpios`

###### 3. Write firmware files on a bootable SD card
* make a directory and mount the filesystem into it
```
sudo mkdir /mnt/usbdrive
sudo mount /dev/sdd1 /mnt/usbdrive
```
* copy the [RPI boot files](https://github.com/raspberrypi/firmware/tree/master/boot). We only need `bootcode.bin`, `start.elf`. The `kernel8.img` we will generate ourselves and put all 3 files on a bootable SD card.
* unmount the card filesystem before remove it
`$ sudo umount /dev/sdd1`


#### Blinking LED
The code above is the simplest skeleton  but it does nothing except for waiting for events in busy loop. We have no opportunities to check that it works well on the real RPI board. Instead of an empty loop lets write someting that is observable in the real world. the simplest example is to turn on the LED via GPIO. 

###### GPIO
GPIO stands for General Purpose Input/Output. As the name implies, GPIO is a general mechanism for transmitting data/signals into and out of some device through electrical pins, known as GPIO pins.

A GPIO pin can act as either output or input. When a GPIO pin is acting as an output, it can either be set ON or OFF. When ON the Raspberry Pi drives the pin at 3.3v. When the GPIO pin is OFF, no current flows through the pin. When a GPIO pin is acting as an input Rasbperry Pi reports whether the pin is being driven at 3.3v or not.

Use GPIO21 (PIN40) and GND (PIN39) to connect the LED. Note that pin-40 on the Raspberry Pi should go to the *longer leg* of your LED. The shorter leg is connected to pin-39 (ground) on the Raspberry Pi.

###### GPIO Memory-Mapped Interface
The vast majority of modern hardware devices communicate with software through *memory-mapped I/O*. The concept is simple: devices expose their functionality through set of registers and provide a specification about what will happen if certain register are read or written to. These registers are memory-mapped. That is, the register has an address in the memory space and can be reffered through its address.

Each GPIO is controlled by three registers (Function Select, Set, Clear). As we are using the GPIO21 to control the LED we need to use these three registers:
| name    | peripheral address | description            | size   | read/write |
| :---    |        :----:      |   :----:               | :----: | ---:       |
| GPFSEL2 |  0x3F200008        | GPIO function select 2 | 32 bit |  R/W       |
| GPSET0  |  0x3F20001C        | GPIO pin turn ON       | 32 bit |    W       |
| GPCLR0  |  0x3F200028        | GPIO pin turn OFF      | 32 bit |    W       |

