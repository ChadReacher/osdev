# Operating system from scratch.
My attempt to make a basic OS from scratch.
The goal of writing this OS is to get practical experience with C, get more knowledge about low level stuff.

# Do not test on real hardware!

## Main features:
* Custom bootloader
* 32 bit mode. Runs on Intel i386
* Devices: keyboard, CMOS RTS, timer, serial ports, tty.
* Video graphics: standard VGA 80x25 text mode
* Virtual memory(paging)
* Userspace programs(ELF loading)
* Small C library
* Multiprocessing support
* Filesystems (ext2, symbolic links, mount points, special device types)

# Build
In order to build you need a cross-compiler(i387-elf-gcc, i386-elf-ld), NASM and qemu(qemu-system-i386).
To build a cross-compiler - run `./build.sh`. This will build binutils and gcc for Intel i386 architecture.
To get other tools, run:
```
sudo apt install make
sudo apt install nasm
sudo apt install qemu-system-i386
```

To build an OS and disk - run:
```
make
```

To start the OS - run `make run`
