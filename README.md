# RPI OS
Experimentation with bare-metal RPI and learning of writing the OS kernel. 
Based on the https://oscapstone.github.io/labs/overview.html 

## Lab 0
The goals of this lab are:
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


## Lab 1
The goals of this lab are:
* Practice with programming of GPIO
* Understand some periperals of RPI
* Setup mini UART
* Setup mailbox


#### start.S - the kernel entry point

```
_start:
  mrs x1, mpidr_el1
  and x1, x1, #3
  cbz x1, 2f
1:
  wfe
  b 1b
```
These are the first instuctions of our kernel. They will send three out of the four cores to infninite loop. One core is enough for our simple kernel.

```
  ldr x1, =_start
  mov sp, x1
```
This says that our C stack should start at address where the kernel is loaded. Since the stack grow downwards it doesn't damage the kernel.

```
  ldr x1, =__bss_start
  ldr w2, =__bss_size
```
This loads the addresses of the start of the BSS section and its size into registers. BSS is where C global variables that are not initialized at compile time are stored. The symbols `__bss_start` and `__bss_size` are defined in the linker script.

```
3:
  cbz w2, 4f
  str xzr, [x1], #8
  sub w2, w2, #1
  cbnz w2, 3b
```
This code is what zeroes out the BSS section. It put the content of the special register xzr (zero register) into the address in x1. Then it decrements the value of the register w2 (which contains the size of BSS section in 8-byte words). This loops until w2 is not zero.

```
  bl kmain
  b 1b

```
This call the function `kmain`. When the function returns it jumps to the label where infinite loop.

#### some notes about linking
* In a linker script, **.** means *current address*, You can assign the current address and also assign things to the current address,
* The sections of a C program
    - **.text** is where executable code goes
    - **.rodata** is *read only data*; it is where global constants are placed
    - **.data** is where global variables that are initialized at compile time are placed
    - **.bss** is where uninitialized global variables are placed

#### Mini UART
The Raspberry Pi boards feature is an almost 16650-compatible UART called the Mini UART.


###### initialization
```
       *AUX_ENABLE |= 1;    // Enable MiniUART
       *AUX_MU_CNTL = 0;    // Disable receiver and transmitter while configuring
       *AUX_MU_LCR = 3;     // Set 8-bit mode
       *AUX_MU_MCR = 0;     // Set RTS line high
       *AUX_MU_IER = 0;     // Disable interrupts, we'll use polling mode
       *AUX_MU_IIR = 6;     // Clear receive FIFO and transmit FIFO
       *AUX_MU_BAUD = 270;  // Set the baud rate 115200
```

Set alternate function 5 for GPIO14 and GPIO15
```
       r = *GPFSEL1;
       r &= ~((7 << 12) | (7 << 15));
       r |= (2 << 12) | (2 << 15);
       *GPFSEL1 = r;
```

Disable pull-up/pull-down control for GPIO14 and GPIO15
```
       *GPPUD = 0;
       r = 150;
       while (r--) { asm volatile("nop"); }
       *GPPUDCLK0 = (1 << 14) | (1 << 15);
       r = 150;
       while (r--) { asm volatile("nop"); }
       *GPPUDCLK0 = 0;
```

When setup is complete, enable receiver and transmitter
```
       *AUX_MU_CNTL = 3;
```

###### send
Wait untill transmitter FIFO is able to accept at least one character and then write character to data register.
```
  do { asm volatile("nop"); } while (!(*AUX_MU_LSR & 0x20));
```

###### receive
Wait untill receiver FIFO holds at leas one symbol and then read it from the data register.
```
       do { asm volatile("nop"); } while(!(*AUX_MU_LSR & 0x01));
       r = (char)(*AUX_MU_IO);
```

#### Mailbox
The mailbox is peripheral which allow bidirectional communication between CPU and GPU. To talk to the GPU, the CPU has to send a mail to the mailbox. Upon receiving the reques, the GPU places the response in another mailbox which the CPU can read to get the requested data.
The mailboxes act as *pipes* between the CPU and the GPU. The peripheral exposes two mailboxes, 0 and 1, to allow full-duplex communication. Each mailbox operates as a FIFO buffer with the capacity to store up to eight 32-bit words. The mailbox 0 alone supports triggering interrupts on the CPU side, which makes it well-suited for CPU-initiated read operations.

###### registers
Each mailbox exposes 5 registers. Their addresses start at offset 0xB880.
| Offset (mb 0/mb 1) |   Purpose  | Description                               |
|--------------------|------------|-------------------------------------------|
|  0x00/0x20         | Read/Write | Access to FIFO                            |
|  0x10/0x30         | Peek       | Read without popping FIFO                 |
|  0x14/0x34         | Sender     | Bits [1:0] specify the sender ID          |
|  0x18/0x38         | Status     | Contains information about the FIFO state |
|  0x1C/0x3C         | Config     | Used for configuring mailbox behaviour    |

###### channels
A channel acts as a descriptor for the asoociated data within a mail. It helps the GPU discern how to process the incoming mail.
| Channel | Name                       |
|---------|----------------------------|
| 0       | Power management           |
| 1       | Framebuffer                |
| 2       | Virtual UART               |
| 3       | VCHIQ                      |
| 4       | LEDs                       |
| 5       | Buttons                    |
| 6       | Touch screen               |
| 8       | Property tags (ARM to GPU) |
| 9       | Property tags (GPU to ARM) |


###### talking to the GPU
The mailbox peripheral is poorly documented. For the purpose of this lab we'll use only one interface - the prorperty interface. This is almost only one interface which is documented.
To query the properties from GPU
1. compose a message.
The message consists of a size, a status field (request, successfull response or failure notification), a list of tags. The list of tags is zero teminated, i.e. the last tag should be zero.
Each of the tag is an individual request which consists of identifier, a size of its value buffer, a place to put the result's size and result buffer itself.
2. compose a mail.
The mail is a fundamental quantum for the information exchange over the mailboxes. A mail encapsulates the data and channel number within a 32-bit word. The channel number to read properties is 8, the data is pointer to the message. Because of we have only 28 bits for data field, the pointer must be 16 bytes aligned.3. put mail into the mailbox
 * in a loop check the status register to see if the FIFO has at least one available spot
 * write mail into the write register
4. get the mail from the mailbox
Thought the mailbox supports interrupts, we will implement the busy-wait fetching to simplification.
 * in a loop read the status register to make sure that FIFO has at least one item 
 * read the mail from the read register
 * compare the mail received with that one which was send before, ignore response it if not our
 * check that status field indicates the successfull response
5. read the values depending on what kind of information was requested


#### references
[RPi mailbox](https://bitbanged.com/posts/understanding-rpi/the-mailbox/)

## Lab 2
The goals of this lab are:
* Implement a bootloader that loads kernel image through UART
* Add an initial ramdisk
* Implement a simple memory allocator
* Add support of device tree

###### load the kernel image through UART
Initially the kernel image is written on SD-card into boot partition. When kernel starts it is loaded into memory and starts execution from predefined address. Then it make attempt to load a new kernel image via UART (the UART must be initialized at that moment). The kernel image is transferred via the simplified implementation of XModem communication protocol:
* send a special command to opposite device which initiates the transfer session.
* the opposite device responds either with XModem packet or special command (not specified by XModem protocol, just my workaround) which tells to RPi to cancel session and continue with current kernel.
* RPi receive packet, check that its sequence number and calculate checksum of data block.
* if packet number match to the expected one and the checksum is correct, RPi send acknowledge command to the kernel sender otherwise the negative acknowledge will be send. 
* when packet is acknowledged the kernel sender send the next packet, otherwise resend the previous one.
* the session is finished after transferring the whole image.

During the transferring session RPi writes the new kernel image to the address in memory which differs from that one at which the currently running kernel started. When the image transferring is finished RPi jumps to a location where new image written and start execution of just received kernel.


##### references
[Linux Serial Ports Using C/C++](https://blog.mbedded.ninja/programming/operating-systems/linux/linux-serial-ports-using-c-cpp/
[Booting your own kernel on RPi via UART](https://blog.nicolasmesa.co/posts/2019/08/booting-your-own-kernel-on-raspberry-pi-via-uart/)
[XModem protocol with CRC](http://ee6115.mit.edu/amulet/xmodem.htm)
