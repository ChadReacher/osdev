SHELL := /bin/bash
TOOLCHAIN_SRC = /usr/bin/i386elfgcc/bin
CC = $(TOOLCHAIN_SRC)/i386-elf-gcc
LD = $(TOOLCHAIN_SRC)/i386-elf-ld
OBJCOPY = $(TOOLCHAIN_SRC)/i386-elf-objcopy
AS = nasm
AR = ar

# TODO: Separate debug and release builds
CFLAGS = -g -W -Wall -pedantic -m32 -std=c11 -march=i386
CFLAGS += -ffreestanding -nostdlib -nostdinc -fno-builtin -nostartfiles
CFLAGS += -nodefaultlibs -mno-red-zone -fno-stack-protector -nolibc
CFLAGS += -fno-omit-frame-pointer
LDFLAGS = -nostdlib

export CC LD OBJCOPY AS AR CFLAGS LDFLAGS

.PHONY: OS image user libc libk run debug log clean clean-deps

all: image user

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
	$(shell mkdir -p userland/hdd/{bin,etc,home,lib,tmp,usr,var,dev})
	mv -f userland/bin/* userland/hdd/bin
	man wc > userland/hdd/home/file
	echo "del" > userland/hdd/home/del
	echo "bye" > userland/hdd/home/bye
	sudo mknod userland/hdd/dev/tty0 c 0x04 0x00
	dd if=/dev/zero of=build/disk.img bs=1024 count=4096
	sudo losetup -fP build/disk.img
	sudo losetup
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
		-drive file=build/boot.img,if=ide,format=raw,media=disk,index=0\
	   	-drive file=build/disk.img,if=ide,format=raw,media=disk,index=1\
		-no-reboot\
		-chardev stdio,id=char0,logfile=serial.log,signal=off\
		-serial chardev:char0

debug:
	qemu-system-i386\
		-drive file=build/boot.img,if=ide,format=raw,media=disk,index=0\
	   	-drive file=build/disk.img,if=ide,format=raw,media=disk,index=1\
		-rtc base=localtime,clock=host,driftfix=slew\
		-boot a -s -S &\
		gdb

clean:
	rm -rf build/* userland/hdd/*
	$(MAKE) clean -C kernel

clean-deps:
	rm -rf build/* userland/hdd/*
	$(MAKE) clean-deps -C kernel

# $@ - target name
# $? - all prerequisites newer than the target
# $^ - all prerequisites
# $< - current prerequisite
# $* - the same as % in target or prerequisite
