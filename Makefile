TOOLCHAIN_SRC = /usr/bin/i386elfgcc/bin

CC = $(TOOLCHAIN_SRC)/i386-elf-gcc
LD = $(TOOLCHAIN_SRC)/i386-elf-ld
OBJCOPY = $(TOOLCHAIN_SRC)/i386-elf-objcopy
AS = nasm
AR = ar

C_FLAGS = -g -W -Wall -pedantic -m32 -std=c11 -ffreestanding -nostdlib -nostdinc -fno-builtin -nostartfiles -nodefaultlibs -mno-red-zone -fno-stack-protector

LIBK = build/libk.a

C_SRC = $(wildcard src/*.c)
HEADERS = $(wildcard src/*.h)
ASM_SRC = src/stage1.asm src/stage2.asm src/font.asm

OBJ_SRC = $(C_SRC:src/%.c=build/%.o)
OBJ_SRC := $(filter-out build/kernel.o, $(OBJ_SRC))
BIN_SRC = $(ASM_SRC:src/%.asm=build/%.bin)

all: OS disk.img

OS: build/bootloader.bin build/kernel.bin 
	cat $^ > build/OS.bin
	dd if=/dev/zero of=build/boot.img bs=512 count=2880
	dd if=build/OS.bin of=build/boot.img conv=notrunc bs=512

disk.img: hdd/init
	dd if=/dev/zero of=disk.img bs=1M count=4096
	cp init/init hdd/init
	mkfs.ext2 -b 1024 -g 8192 -i 1024 -r 0 -d hdd disk.img
	@echo "HDD has been created with EXT2 file system(revision 0). It has 1024 block size(bytes) and 8192 blocks per block group" 

hdd/init: $(LIBK)
	cd init && make clean && make

build/kernel.bin: build/kernel.elf
	$(OBJCOPY) -O binary $^ $@		

build/bootloader.bin: $(BIN_SRC)
	cat $^ > build/bootloader.bin

build/kernel.elf: build/kernel_entry.o build/kernel.o build/interrupt.o $(LIBK)
	$(LD) -Tkernel_linker.ld $^ -o $@

$(LIBK): $(OBJ_SRC)
	$(AR) rcs $@ $^

run:
	qemu-system-i386 -drive format=raw,file=build/boot.img,if=ide,index=0,media=disk\
	       	-drive file=disk.img,if=ide,format=raw,media=disk,index=1\
		-rtc base=localtime,clock=host,driftfix=slew

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
	       	-boot a -s -S \
		& gdb -ex "target remote localhost:1234" -ex "symbol-file build/kernel.elf"\
	       	-ex "br _start" -ex "layout src" -ex "continue" -ex "next"\

build/interrupt.o: src/interrupt.asm
	$(AS) -f elf32 $< -o $@

build/kernel_entry.o: src/kernel_entry.asm
	$(AS) -f elf32 $< -o $@

build/%.o: src/%.c
	$(CC) $(C_FLAGS) -c $< -o $@

build/%.bin: src/%.asm
	$(AS) -f bin $< -o $@

clean:
	rm -f build/* 
