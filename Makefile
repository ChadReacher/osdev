TOOLCHAIN_SRC = /usr/bin/i386elfgcc/bin

CC = $(TOOLCHAIN_SRC)/i386-elf-gcc
LD = $(TOOLCHAIN_SRC)/i386-elf-ld
OBJCOPY = $(TOOLCHAIN_SRC)/i386-elf-objcopy
AS = nasm
AR = ar
C_FLAGS = -g -W -Wall -pedantic -m32 -std=c11 -ffreestanding -nostdlib -nostdinc -fno-builtin -nostartfiles -nodefaultlibs -mno-red-zone -fno-stack-protector\
		  -I ./include/ -I ./libk

LIBC = build/libc.a
LIBK = build/libk.a

LIBC_OBJ = $(shell find libc/ -type f -name "*.c")
LIBC_OBJ := $(LIBC_OBJ:libc/%.c=%.c)
LIBC_OBJ := $(LIBC_OBJ:sys/%.c=%.c)
LIBC_OBJ := $(LIBC_OBJ:%.c=build/libc/%.o)
LIBK_OBJ = build/libk/stdio.o build/libk/stdlib.o build/libk/string.o build/libk/ctype.o

HEADERS = $(wildcard include/*.h)
ASM_SRC = src/boot/stage1.asm src/boot/stage2.asm src/boot/font.asm

C_SRC += $(wildcard src/core/*.c)
C_SRC := $(C_SRC:src/core/%.c=%.c)

C_SRC += $(wildcard src/dev/*.c)
C_SRC := $(C_SRC:src/dev/%.c=%.c)

C_SRC += $(wildcard src/fs/*.c)
C_SRC := $(C_SRC:src/fs/%.c=%.c)

C_SRC += $(wildcard src/ds/*.c)
C_SRC := $(C_SRC:src/ds/%.c=%.c)

C_SRC += $(wildcard src/mmu/*.c)
C_SRC := $(C_SRC:src/mmu/%.c=%.c)

C_SRC += $(wildcard src/proc/*.c)
C_SRC := $(C_SRC:src/proc/%.c=%.c)

KERNEL_OBJECTS = $(C_SRC:%.c=build/%.o)
KERNEL_OBJECTS := $(filter-out build/kernel.o, $(KERNEL_OBJECTS))
BIN_SRC = $(ASM_SRC:src/boot/%.asm=build/%.bin)

dirs = src/core src/dev src/ds src/fs src/mmu src/proc
mkfiles = $(patsubst %, %/Makefile, $(dirs))

# IT WORKS
#C_SOURCES = $(shell find src/ -type f -name '*.c' ! -path "src/.*/*")
#OBJECTS = ${C_SOURCES:src/%c=build/%o}
#
#all: prepare #OS
#
#build/%.o: src/%.c
#	@echo "input - " $< " output - " $@
#	@#$(CC) -g -W -Wall -pedantic -m32 -std=c11 -ffreestanding -nostdlib -nostdinc -fno-builtin -nostartfiles -nodefaultlibs -mno-red-zone -fno-stack-protector -I ./include/ -c $< -o build\$@
#
#.PHONY: prepare
#prepare: $(OBJECTS)
#	@echo $(C_SOURCES)
#	@echo $(OBJECTS)
#	mkdir -p build/libc
#	mkdir -p build/libk

all: prepare OS

.PHONY: prepare
prepare: 
	mkdir -p build/libc
	mkdir -p build/libk

.PHONY: OS
OS: build/bootloader.bin build/kernel.bin
	cat $^ > build/OS.bin
	dd if=/dev/zero of=build/boot.img bs=512 count=2880
	dd if=build/OS.bin of=build/boot.img conv=notrunc bs=512

.PHONY: disk
disk: userland
	dd if=/dev/zero of=disk.img bs=1M count=4096
	mkfs.ext2 -b 1024 -g 8192 -i 1024 -r 0 -d userland/hdd/ disk.img
	@echo "HDD has been created with EXT2 file system(revision 0). It has 1024 block size(bytes) and 8192 blocks per block group" 

.PHONY: userland
userland: $(LIBC)
	mkdir -p userland/bin
	$(MAKE) -C userland
	cp -r userland/bin userland/hdd/

build/bootloader.bin: $(BIN_SRC)
	cat $^ > build/bootloader.bin

build/%.bin: src/boot/%.asm
	$(AS) -f bin $< -o $@

build/kernel.bin: build/kernel.elf
	$(OBJCOPY) -O binary $^ $@		

build/kernel.elf: build/kernel_entry.o build/kernel.o build/interrupt.o $(KERNEL_OBJECTS) build/gdt_helper.o build/tss_helper.o build/context_switch.o $(LIBK)
	$(LD) -Tkernel_linker.ld $^ -o $@

build/kernel_entry.o: src/boot/kernel_entry.asm
	$(AS) -f elf32 $< -o $@

build/kernel.o: src/kernel.c
	$(CC) $(C_FLAGS) -c $< -o $@

build/interrupt.o: src/boot/interrupt.asm
	$(AS) -f elf32 $< -o $@

build/gdt_helper.o: src/core/gdt_helper.asm
	$(AS) -f elf32 $< -o $@

build/tss_helper.o: src/core/tss_helper.asm
	$(AS) -f elf32 $< -o $@
	
build/context_switch.o: src/boot/context_switch.asm
	$(AS) -f elf32 $< -o $@

$(KERNEL_OBJECTS): $(dirs)

$(dirs): $(mkfiles)
	$(MAKE) -C $@

$(LIBK): $(LIBK_OBJ)
	$(AR) -rcs $@ $^

build/libk/%.o: libk/%.c
	$(CC) -g -W -Wall -pedantic -m32 -std=c11 -ffreestanding -nostdlib -nostdinc -fno-builtin -nostartfiles -nodefaultlibs -mno-red-zone -fno-stack-protector -I ./include/ -c $< -o $@

$(LIBC): $(LIBC_OBJ)
	$(AR) -rcs $@ $^

build/libc/%.o: libc/%.c
	$(CC) -g -W -Wall -pedantic -m32 -std=c11 -ffreestanding -nostdlib -nostdinc -fno-builtin -nostartfiles -nodefaultlibs -mno-red-zone -fno-stack-protector -I ./include/ -c $< -o $@

build/libc/%.o: libc/sys/%.c
	$(CC) -g -W -Wall -pedantic -m32 -std=c11 -ffreestanding -nostdlib -nostdinc -fno-builtin -nostartfiles -nodefaultlibs -mno-red-zone -fno-stack-protector -I ./include/ -c $< -o $@

run:
	qemu-system-i386 -drive format=raw,file=build/boot.img,if=ide,index=0,media=disk \
	    -drive file=disk.img,if=ide,format=raw,media=disk,index=1\
		-rtc base=localtime,clock=host,driftfix=slew #\
		#-netdev tap,id=net1,ifname=tap,script=no,downscript=no -device rtl8139,netdev=net1 -object filter-dump,id=f1,netdev=net1,file=dump.dat
		#-device rtl8139,netdev=net0\
		#-netdev user,id=net0\
		#-object filter-dump,id=net0,netdev=net0,file=dump.dat

log:
	qemu-system-i386 -drive format=raw,file=build/boot.img,if=ide,index=0,media=disk\
		-drive file=disk.img,if=ide,format=raw,media=disk,index=1\
		-rtc base=localtime,clock=host,driftfix=slew\
		-d int -no-reboot\
		-chardev stdio,id=char0,logfile=serial.log,signal=off\
		-serial chardev:char0

debug:
	qemu-system-i386 -drive format=raw,file=build/boot.img\
		-drive file=disk.img,if=ide,format=raw,media=disk,index=1 \
		-boot a -s -S &\
		gdb -ex "target remote localhost:1234" -ex "symbol-file build/kernel.elf"\
	       	-ex "br _start" -ex "layout src" -ex "continue" -ex "next"

.PHONY: clean
clean:
	rm -rf build/* disk.img userland/bin/* userland/hdd/bin/*
