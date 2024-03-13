TOOLCHAIN_SRC = /usr/bin/i386elfgcc/bin
CC = $(TOOLCHAIN_SRC)/i386-elf-gcc
LD = $(TOOLCHAIN_SRC)/i386-elf-ld
OBJCOPY = $(TOOLCHAIN_SRC)/i386-elf-objcopy
AS = nasm
AR = ar
CFLAGS = -g -W -Wall -pedantic -m32 -std=c99 -march=i386
CFLAGS += -ffreestanding -nostdlib -nostdinc -fno-builtin -nostartfiles
CFLAGS += -nodefaultlibs -mno-red-zone -fno-stack-protector -nolibc

export CC LD OBJCOPY AS AR CFLAGS

.PHONY: OS image user libc libk run debug log clean clean-all

all: OS

OS: image user

image: build/bootloader.bin build/kernel.bin
	cat $^ > build/OS.bin
	dd if=/dev/zero of=build/boot.img bs=512 count=2880
	dd if=build/OS.bin of=build/boot.img conv=notrunc bs=512

build/bootloader.bin:
	$(MAKE) -C boot

build/kernel.bin: libk
	$(MAKE) -C kernel

user: libc
	$(MAKE) -C userland
	mkdir -p userland/hdd/bin
	mv -f userland/bin/* userland/hdd/bin
	mkdir -p userland/hdd/usr
	man bash > userland/hdd/usr/file
	echo "del" > userland/hdd/usr/del
	echo "bye" > userland/hdd/usr/bye
	#mkdir -p userland/hdd/usr/etc
	#echo "x" > userland/hdd/usr/etc/x
	#./generate.sh
	dd if=/dev/zero of=build/disk.img bs=1024 count=4096
	sudo losetup -fP build/disk.img
	losetup
	sudo ./disk_part.sh
	sudo mkfs.ext2 -b 1024 -g 1024 -r 0 -d userland/hdd/ /dev/loop0p1 
	sudo dumpe2fs /dev/loop0p1
	sudo losetup -d /dev/loop0

libk:
	mkdir -p build/libk
	$(MAKE) -C libk

libc:
	mkdir -p build/libc
	$(MAKE) -C libc
	
run:
	qemu-system-i386\
		-drive file=build/boot.img,if=ide,format=raw,media=disk,index=0\
	    -drive file=build/disk.img,if=ide,format=raw,media=disk,index=1\
		-rtc base=localtime,clock=host,driftfix=slew

log:
	qemu-system-i386\
		-drive format=raw,file=build/boot.img,if=ide,index=0,media=disk\
		-drive file=build/disk.img,if=ide,format=raw,media=disk,index=1\
		-rtc base=localtime,clock=host,driftfix=slew\
		-d int -no-reboot\
		-chardev stdio,id=char0,logfile=serial.log,signal=off\
		-serial chardev:char0

debug:
	qemu-system-i386\
		-drive format=raw,file=build/boot.img\
		-drive file=build/disk.img,if=ide,format=raw,media=disk,index=1 \
		-rtc base=localtime,clock=host,driftfix=slew\
		-boot a -s -S &\
		gdb

clean:
	rm -rf build/* userland/hdd/*
	$(MAKE) clean -C kernel

clean-all:
	rm -rf build/* userland/hdd/*
	$(MAKE) clean-all -C kernel

# $@ - target name
# $? - all prerequisites newer than the target
# $^ - all prerequisites
# $< - current prerequisite
# $* - the same as % in target or prerequisite
