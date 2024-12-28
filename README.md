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

#### Load the kernel image through UART
Initially the kernel image is written on SD-card into boot partition. When kernel starts it is loaded into memory and starts execution from predefined address. Then it make attempt to load a new kernel image via UART (the UART must be initialized at that moment). The kernel image is transferred via the simplified implementation of XModem communication protocol:
* send a special command to opposite device which initiates the transfer session.
* the opposite device responds either with XModem packet or special command (not specified by XModem protocol, just my workaround) which tells to RPi to cancel session and continue with current kernel.
* RPi receive packet, check that its sequence number and calculate checksum of data block.
* if packet number match to the expected one and the checksum is correct, RPi send acknowledge command to the kernel sender otherwise the negative acknowledge will be send. 
* when packet is acknowledged the kernel sender send the next packet, otherwise resend the previous one.
* the session is finished after transferring the whole image.

During the transferring session RPi writes the new kernel image to the address in memory which differs from that one at which the currently running kernel started. When the image transferring is finished RPi jumps to a location where new image written and start execution of just received kernel.

The jump to address is implemented with `br` instruction.
```
branch_to_address:
    br x0
```

#### Initial ramdisk
Initial ramdisk is a ram-based block device, that is a simulated hard disk that uses memory instead of physical disk. Before the kernel boot the ramdisk is populated from CPIO archive. CPIO is pretty old (since 1990) but convenient way of packing files, directories and other filesystem objects into a sinngle contiguous bytestream.
**CPIO format**
Each filesystem object in archive consists of a header with basic numeric metadata which followed by the full path to the object and content of this object. The end of the archive is marked by special object with name 'TRAILER!!!'. There are different kind of header formats. One of them is 'New ASCII Format', it uses 8-byte hexadecimal fields for all numbers. The man

```
struct cpio_newc_header {
»       char c_magic[6];
»       char c_ino[8];
»       char c_mode[8];
»       char c_uid[8];
»       char c_gid[8];
»       char c_nlink[8];
»       char c_mtime[8];
»       char c_filesize[8];
»       char c_devmajor[8];
»       char c_devminor[8];
»       char c_rdevmajor[8];
»       char c_rdevminor[8];
»       char c_namesize[8];
»       char c_check[8];
};

```
The pathname is followed by NUL bytes so that the total size of the fixed header plus pathname is a multiple of four. Likewise, the file data is padded to a multiple of four bytes.

To apply this knowledge in practice I'm going to write two functions: the first one of them reads content of CPIO archive and print it, the second function prints the content of a file specified with its name.

The both functions are working by the same principle:
* read the header
* get the length of a filepath and the length of a file
* read the filepath (and content of the file if needed)
Repeat these steps untill reach the object with reserved name 'TRAILER!!!'.

The initial implementation of this function called on the service function of the  UART module. It is not good from design point of view and I rewrote both functions so that they invoke a callback function which handle symbols which are read from ramdisk (either filepath or content of a file). The prototypes are:
```
void cpio_read_catalog(putchar_cb_t putchar_cb);
int cpio_read_file(const char *fname, putchar_cb_t putchar_cb);
```
And an example of usage:
```
void list_files()
{
»       cpio_read_catalog(&uart_putc);
}
```


There are two ways to load the CPIO archive into memory:
* the simple way is *statically link*. The ramdisk content is accessible in code by the label *_binary_ramdisk_start*.
* ask the GPU to load ramdisk with one of the two options of the `config.txt`
    - `ramfsfile=(filename)` - will load the file after kernel image. The ramdisk will be accessible at the label *_end* defined by linker script.
    - `initramfs (filename) (address)` - will load the file into a specified location. The ramdisk will be accessible at *address*.

Initially I chose the statically linking. I needed to make some changes in the `Makefile` (ramdisk is name of the file which contains CPIO archive):
```
rd.o: ramdisk
»       $(CROSS)ld -r -b binary -o rd.o ramdisk

...
kernel8.elf: start.o utils.o $(OBJS) rd.o kernel.ld
»       $(CROSS)ld -nostdlib -nostartfiles start.o rd.o utils.o $(OBJS) -T kernel.ld -o $@

```

Later, when I opted to loading the ramdisk by the GPU, these lines in the `Makefile` lost sense and I removed them.
But now I need to copy the file `ramdisk` to the SD-card, add the following line into the file `config.txt`

```
initramfs ramdisk 0x20000
```
and use the specified address in the code which read the ramdisk content.


#### Memory allocator
Memory allocator is a mechanism which allows an allocation of memory block of arbitrary size. Its a vaste topic to discuss, for this lab just make it as simple as possible. Our allocator should provide a function which returns a pointer to memory block of requested size. The memory block has to be contigous. The memory blocks will be allocated from the area following immediatelly the BSS serction. We  can get the starting address of this area using the symbol `_end` that declared in the linker script.
Every time when the memory block is requested the current pointer will be incremented by the requested size.
```
void* malloc(unsigned size)
{
»       if (heap == 0L)
»       »       heap = &_end;

»       unsigned char *p = heap;
»       heap += size;
»       return p;
}
```

Let's rewrite function which read a file from CPIO archive. Instead of callback function which print the symbols it would accept address of the buffer start and the capacity of that buffer.
```
int cpio_read_file(const char *fname, char *buffer, int buffer_size);
```

The client function `void print_file(const char *fname)` will query the filesize, allocate a buffer to read file content, call the function reading file content into buffer and finally print that content.
```
»       int fsize = cpio_file_size(fname);
»       if (fsize < 0) {
»       »       uart_puts("file not found\r\n");
»       »       return;
»       }

»       char *buffer = malloc(fsize);
»       memset(buffer, 0, fsize); 
»       if (cpio_read_file(fname, buffer, fsize) < 0) {
»       »       uart_puts("could not read file\r\n");
»       »       return;
»       }

»       for (int i = 0; i < fsize; i++) {
»       »       uart_putc(buffer[i]);
»       }
»       uart_putc('\r');
»       uart_putc('\n');
```

#### Device tree
A device tree is a hierarchical data structure ofter used to describe the hardware components and their configuration in a system. A device tree source (DTS) is a human-readable representation of the tree. The device tree compiler (DTC) is used to convert the DTS to a binary device tree blob (DTB) that can be used by software like a bootloader of the kernel.
The figure below shows the layout of the blob of data containing the device tree. It has three sections of variable size: the *memory reservation table*, ythe *structure block* and the *strings block*. A small header gives the blob's size and version and location of the three sections,

           *offset*

            0x00 ---------------------
                 | magic number      |
            0x04 |-------------------|
                 | total size        |
            0x08 |-------------------|
                 | off dt struct     |
            0x0C |-------------------|
                 | off dt strings    |
            0x10 |-------------------|
                 | off mem rsvmap    |
            0x14 |-------------------|
                 | version           |
            0x18 |-------------------|
                 | last comp version |
            0x1C |-------------------|
                 | boot cpuid phys   |
            0x20 |-------------------|
                 | size dt strings   |
            0x24 |-------------------|
                       ......         
                       ......         
  off mem rsvmap |-------------------|
                 |      memory       |
                 | reservation block |
                 |-------------------|
                       ......         
                       ......         
   off dt struct |-------------------|
                 |                   |
                 |  structure block  |
                 |                   |
                 |-------------------|
                       ......         
                       ......         
  off dt strings |-------------------|
                 |                   |
                 |   strings block   |
                 |                   |
                 |-------------------|

The memory reserve map section gives a list of regions of memory that the kernel must not use. The list is represented as a simple array of (address, size)) pairs of 64 bit values, terminated by a zero size entry. The strings block is similarly simple, consisting of a number of null-terminated strings appended together, which are referenced from the structure block as described below.
The structure block contains the device tree nodes. Each node is introduced with a 32-bit DT_BEGIN_NODE flag, followed bu the node's name as a null-terminated string, padded to a 32-bit boundary. Then follows all of the properties of the node, each introduced with a DT_PROP tag, then all of the node's subnodes, each introduced with their own DT_BEGIN_NODE tag. The node ends with an DT_END_NODE tag, and after the DT_END_NODE for the root node is an DT_END tag, indicating the end of the whole tree. 
Each property, after the DT_PROP tag, has a 32-bit value giving an offset from the beginning of the strings block at which the property name is stored. The name offset is followed by the length of the property value (as a 32-bit value) and then the data itself padded to a 32-bit boundary.

##### references
* [Linux Serial Ports Using C/C++](https://blog.mbedded.ninja/programming/operating-systems/linux/linux-serial-ports-using-c-cpp/)
* [Booting your own kernel on RPi via UART](https://blog.nicolasmesa.co/posts/2019/08/booting-your-own-kernel-on-raspberry-pi-via-uart/)
* [XModem protocol with CRC](http://ee6115.mit.edu/amulet/xmodem.htm)
* [initramfs buffer format](https://docs.kernel.org/driver-api/early-userspace/buffer-format.html)
* [Flattened Devicetree (DTB) Format](https://devicetree-specification.readthedocs.io/en/stable/flattened-format.html)
* [Device tree everywhere](https://ozlabs.org/~dgibson/papers/dtc-paper.pdf)

## Lab 3 
The goals of this lab are:
* Understand what's exception levels in Armv8-A
* Understanding what's exception handling
* Understand what's interrupt
* Understand how RPi3's peripherals interrupt the CPU by interrupt controllers
* Understand how to multiplex a timer
* Understand how to concurrently handle I/O devices

#### Exception levels in Armv8-A
ARM specification defines four priviledge levels that are called Exception Levels and numbered from 0 to 3, where **EL0** is the lowest priviledge level and **EL3** is the highest priviledge level.
The rough purpose of each EL are as follows:
* EL3 the highest priviledge level is typically used for so called **Secure Monitor**.
* EL2 seem to target the virtualization use-case specifically and that's the EL at which hypervisors would normally use for virtualization purposes.
* EL1 is th level that priviledged parts of the OS kernel use, so for example, Linux Kernel code will run with EL1 priviledges.
* EL0 is the most unpriviledged level and therefor that's where the most unpriviledged code runs (userspace applications, userspace drivers, etc).

##### Jumping between ELs
ARM specification is pretty clear that there are only two ways to change the EL:
* take an exception/interrupt - this may switch CPU from a lower EL to a higher EL.
* return from an exception/interrupt - this may switch CPU from a higher EL to a lower EL.

ARM has a specific instruction used to return from an interrupt: `eret`. This instruction can be executed even if there was no interrupt. To jump to a lower level  with instruction `eret` we have to prepare all the state that `eret` instruction needs and run the `eret` instruction without an actual interrupt or exception.
The state required by the `eret` instruction is stored in a few registers. For example, when running on EL2 and executing `eret` instruction takes the address of the instruction to return to from the `ELR_EL2` register and some other state of the processor from `SPSR_EL2` register. Among other things `SPSR_EL2` register contains the EL to which the `eret` instruction should switch the processor to.

##### Switching to EL1
When RPi starts it runs on EL2 by default and our bootstrap procedure have to jump to EL1 to run our minimal OS on it. Strictly speaking, our OS is not obligued to switch to EL1 but EL1 is a natural choice for us because this level has just the right set of privileges to implement all common OS tasks. Let's take a look at the code that does it:
```
  mov x2, (1 << 31)
  msr hcr_el2, x2
  mov x2, 0x3c5
  msr spsr_el2, x2
  ldr x2, =el1_entry
  msr elr_el2, x2
  eret
```

What this code does:
1. Set that execution state at EL1 will be AArch64 but not the AArch32 (by setting the bit 31 at **HCR_EL2**, Hypervisor Conficuration Register).
2. Set the exception level to switch to by writing a constant 0x3c5 into the **SPSR_EL2** (Saved Program State Register). The last four bits define the exception level and selected SP. The value 0b0101 means EL1h, i.e. switch to EL2 usinf dedicated stack pointer, The other bits mask exception and interrupts.
3. Set the address to which we are going to return after `eret` instruction will be executed. Here the address of the location of `el1_entry` label is written into **ELR_EL2** (Exception Link Register).


##### Launch a userprogram
A userprogram in this context is just binary code which comprise  a sequence of instructions forming execution flow of the program.
To run the program in our mini OS we need to switch to the exception level EL0 and continue execution from the start point of that program. Before swtching to EL0 we need to allocate stack to it.
I placed a code to run a user program in dedicated function called `execute_at_el0`. It takes two arguments: the address of program to execute and address of the top of the applications stack.
```
execute_in_el0:
    save_context
    mov x2, 0x3c0
    msr spsr_el1, x2
    msr sp_el0, x1
    msr elr_el1, x0
    eret
```

The code set the target EL with masked exceptions and interrupts, set the address to continue from when the `eret` is finished and set address of stack for the target EL (EL0). Finally, instruction `eret` tells to processor to load saved (prepared by us) state and continue execution from saved address. Just remind that to pass parameters to the function in accordance with AAPCS (Procedure Call Standard for the Arm Architecture) the registers **X0-X7** are used. So the register **X0** contains the first parameter (entrypoint of userprogram) and the register **X1** contains the second parameter (address of applications stack). `save_context` is just a macro which will be explained later.

The program to execute is just a few instructions. The only possible way to return from application level to kernel is to take exception. The program do it with instruction `svc` which is used for system calls.
```
.section ".text"
.globl userprogram
userprogram:
    mov x0, 0
1:
    add x0, x0, 1
    svc 0
    cmp x0, 5
    blt 1b
1:
    b 1b
```
At the initial stage (when debugging) I linked this program with kernel and launch it as following:
```
#define USER_STACK_SIZE 512

extern void userprogram();
...
uint8_t *stack = malloc(USER_STACK_SIZE);
execute_in_el0(&userprogram, stack);
...
```
Later I link it as a standalone ELF-file, strip the ELF-header (just the same as I prepare the kernel image), copy to the ramdisk and launch the program from it: the function `cpio_exec_file(const char *fname, void *stack)` scans the rootdirectory of ramdisk for the givenfilename, takes the address where the file begins and run it with call `execute_in_el0(pfile, stack);` the `pfile` contains address on ramdisk where the file begins.

To return from the program back to the kernel it is needed to switch exception level and continue execution from the address where the execution flow was interrupted. The userprogram generates synchronous exception with command `svc`. The corresponding handler restore the previous context and execute command `ret' which copy the address of the next command from **LR/X30** into **PC**.
```
svc_el0_64:
    load_context
    ret
```
The handler just restore execution context (saved previously by `save_context` in function `execute_in_el0`) and execute instruction `ret`. Whenever the exception handler usually finished by `eret` instruction, which used to switch from higher exception level to lower one (decreasing privileges). What the `eret` actually does when executing on the level ELx:
* set PC to the value kept by ELR_ELx
* set PSTATE to the value kept by SPSR_ELx
In our case we trap into the handler from lower level (application requested synchronous exception with `svc` instruction) but opposite to usual cases we don't need to return to application. We want to return to the kernel code and continue execution immediatelly after the function `execute_in_el0`. How does it work, step by step:
1. Kernel code call the function `execute_in_el0', the return address saved in the link register (LR).
2. The function 'execute_in_el0` save the current execution context (the registers X0-X30) on the stack of the current exception level (EL1). After that the function writes the start address of user program and address of the top of user-stack into special registers and switch to the lower level (EL0) with instruction `eret`. We are not going to return here when the application will finish it's execution.
3. Application does what it needs to and execute instruction `svc` which requested the synchronous exception. Processor handling this command starts execution the correspoinding handler. Now execution level is EL1 but we need to restore processor state as it was before the switching to application. Because of we are at the level EL1 and its stack wasn't changed we can load the previous state with macro `load_context` and return to kernel with command `ret` because LR now points to the address immediatelly after the call `execute_in_el0`.

It is important to note that we store execution context on the stack of EL1 and therefore have to restore that context when being at the EL1.

#### Core Timer
ARMv8 has an architecture-defined per-core timers which are part of a Generic Timer.


                  --------------------
                  |  System counter  |
                  --------------------
                           |
                           | System Timer Bus
                           |
                           v
               -------------------------
               |                       |
               v                       v
          -----------             -----------    Private Peripheral Interrupt
       ---|  Timer  |             |  Timer  |---
       |  -----------             -----------  |
       |  -----------             -----------  |
       |  |         |             |         |  |
       |  |   PE    |             |   PE    |  |
       |  |         |             |         |  |
       |  -----------             -----------  |
       |                                       |
       |      --------------------------       |
       ------>|  Interrupt Controller  |<-------
              --------------------------

The System Counter is an always-on device, which provides a fixed-frequency incrementing system counter. The system count value is broadcast to all the cores in the system. Each core has a set of timers. These timers are comparators, which compare against the broadcast system count that is provided by System Counter. These timers could be configured to generate interrupts in set points in the future.
The `CNTPCT_EL0` system register reports the current system timer value. The `CNTFRQ_EL0` contains the frequency of the system count, the register is writable.

##### Timer registers
* CNTP_CTL_EL0 - control register
* CNTP_CVAL_EL0 - comparator value
* CNTP_TVAL_EL0 - timer value

##### Configuring a timer
To setup timer to trigger an interrupt we need to set bit 0 (ENABLE) and clear bit 1 (IMASK) of the control register.
Timer triggers when the count reaches or exceeds the value written in comparator register, i.e. the condition is met:
`CVAL <= System Count`

The comparator register could be populated either by directly writting the valueto compare into comparator register or writing the value of delay into timer register. In the latter case the comparator register will be populated by the value `CVAL = TVAL + System Count`.


The code to initialize the core timer (at core 0) and setup the next event will trigger in one second is  below:
```
.global core_timer_init
core_timer_init:
  mov x0, 0x1
  msr cntp_ctl_el0, x0
  mrs x0, cntfrq_el0
  msr cntp_tval_el0, x0
  mov x0, 0x2
  ldr x1, =0x40000040
  str x0, [x1]
  ret
```

Setting the bit 1 in the register at address 0x40000040 (Core 0 Timers interrupt control) will enable the interrupts from nonsecure physical timer on core 0. To identify that interrupt need to test the corresponding bit in the register at address 0x40000060 (Core 0 interrupt source). The fragment of IRQ handling code:
```
#define CORE0_IRQ_SOURCE ((volatile uint32_t *)0x40000060)
...
»       uint32_t core0_irq_source = *CORE0_IRQ_SOURCE;
»       if (core0_irq_source & 0x2) {
»       »       core_timer_irq_handler();
»       }
```

Once the timer condition is reached, the timer will continue to signal an interrupt until either one of these:
* IMASK bit is set, which masks the interrupt
* ENABLE bit is cleared, which disables the timer
* TVAL or CVAL is written, so that firing condition is no longer met

The function `core_timer_irq_handler()` just write the timer frequency value into the timer register so clearing the current interrupt and set the 1s delay.

#### Peripheral Interrupt
Will consider interrupts from mini UART as an example of peripheral interrupts. For now we are using polling mode to communicate with UART peripherral, i.e. to send/receive character it is needed to check state of transmitter/receiver FIFO by reading the data status register (0x3F215054). It might be not well in some cases because CPU core wasting time in a busy loop. The alternative approach would be using interrupts. The mini UART peripheral could generate interrupts when:
* receiver FIFO has at least one character
* transmitter FIFO has at leas one slot

Both of these interrupts could be configured independently.
To enable receiver/transmitter interrupts it is needed to set bits corresponding bits (bit 0 to enable receiver interrupts, bit 1 to enable transmitter interrupts) in the interrupt enable register (0x3F215044) of mini UART. The interrupt handler should read the mini UART interrupt status register (0x3F215048) to verify that source of pending IRQ is mini UART and get reason of this interrupt. The relevant bits:
* bit 0 is clear when interrupt pending
* bits 2:1 contain the interrupt ID: 00 - no interrups, 01 - transmitter buffer empty, 10 - receiver has a byte

##### Circular buffer
Interrupts are asynchronous events to the program (mini kernel in our case) which uses UART to communication with external world. It means that we need to keep somewhere the received bytes until kernel will need them, the same related to bytes pending to send - they should be kept somewhere until UART will notify our kernel that transmitter FIFO of mini UART has some space. A commonly known solution to this problem is using circular buffer (ring buffer). The same as UART has two hardware FIFO, our UART driver need two software buffers - one to keep received characters, another one - to store characters pending to send.

##### Interrupt handler
When interrupt handler processing an interrupt from mini UART it should distinguish was it an interrupt from receiver or interrupt from transmitter. When transmitter raise interrupt, the processing code take a symbol from driver's buffer and write it into data register of mini UART. When receiver raise interrupt, the processing code read a symbol from data register and put it into driver's buffer.

  ------------------------
  | mini UART peripheral |
  ------------------------
  |                      |                  -------------------------
  |   -------------      |                  |      UART driver      |
  |   |  Rx FIFO  |      |                  -------------------------
  |   -------------      |                  |                       |
  |        |             |                  |        -------------  |
  |        v             |                  |  ----->| Rx buffer |  |
  |   -----------------  |  irq processing  |  |     -------------  |
  |   | Data Register |-------------------------                    |
  |   |               |<------------------------                    |
  |   -----------------  |                  |  |     -------------  |
  |        |             |                  |  ------| Tx buffer |  |
  |        v             |                  |        -------------  |
  |   -------------      |                  |                       |
  |   |  Tx FIFO  |      |                  -------------------------
  |   -------------      |
  |                      |
  ------------------------

The interrupts in RPI are level-based, it means that interrupt request will remain active untill some actions was not done. When processing interrupts from UART, the reading or writing of the data register (depending on the source of interrupt - either receiver or transmitter) will clear interrupt request. This peculiarity brought me a trouble. I found that UART generates interrupts continuously because the transmitter buffer is empty, but I have no data to send in driver's buffer and my kernel got stuck in repeatedly processing of irq request from UART transmitter. I've not fiund a solution yet and returned to the polling mode.


##### references
* [AArch64 Exception Levels](https://krinkinmu.github.io/2021/01/04/aarch64-exception-levels.html)
* [Interrupts](https://s-matyukevich.github.io/raspberry-pi-os/docs/lesson03/rpi-os.html)
* [AArch64-Reference-Manual](https://developer.arm.com/documentation/ddi0487/ca/)
