#Operating system from scratch.
My attempt to make a basic OS from scratch.
The goal of writing this OS is to get practical experience with C, get more knoledge about low level stuff.

# Do not test on real hardware!

## Main features:
* Custom bootloader
* 32 bit mode
* Device: keyboard, CMOS RTS, timer, video graphics(VESA), serial ports
* Virtual memory(paging)
* Userspace programs(ELF loading)
* Small C library
* Multiprocessing
* Virtual File System
* EXT2 file system

#Build
In order to build you need a cross-compiler(i386-elf-gcc, i386-elf-ld), NASM and qemu(qemu-system-i386)
```
	make
	make disk
	make run
```
