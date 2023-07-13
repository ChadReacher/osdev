TOOLCHAIN_SRC = /usr/bin/i386elfgcc/bin

CC = $(TOOLCHAIN_SRC)/i386-elf-gcc
LD = $(TOOLCHAIN_SRC)/i386-elf-ld
AS = nasm
AR = ar

C_FLAGS = -W -Wall -pedantic -std=c11 -ffreestanding -m32 -nostdlib -nostdinc -fno-builtin -nostartfiles -nodefaultlibs -mno-red-zone -fno-stack-protector

LIBK = build/libk.a

C_SRC = $(wildcard src/*.c)
HEADERS = $(wildcard src/*.h)
ASM_SRC = src/stage1.asm src/stage2.asm src/font.asm

OBJ_SRC = $(C_SRC:src/%.c=build/%.o)
OBJ_SRC := $(filter-out build/kernel.o, $(OBJ_SRC))
BIN_SRC = $(ASM_SRC:src/%.asm=build/%.bin)

all: OS

OS: build/bootloader.bin build/kernel.bin
	cat $^ > build/OS.bin
	dd if=/dev/zero of=build/boot.iso bs=512 count=2880
	dd if=build/OS.bin of=build/boot.iso conv=notrunc bs=512

build/bootloader.bin: $(BIN_SRC)
	cat $^ > build/bootloader.bin

build/kernel.bin: build/kernel.o build/interrupt.o $(LIBK)
	$(LD) --nmagic --output=$@ --script=kernel_linker.ld $^

$(LIBK): $(OBJ_SRC)
	$(AR) rcs $@ $^

run:
	qemu-system-i386 -drive format=raw,file=build/boot.iso,if=ide,index=0,media=disk -rtc base=localtime,clock=host,driftfix=slew

log:
	qemu-system-i386 -drive format=raw,file=build/boot.iso,if=ide,index=0,media=disk -rtc base=localtime,clock=host,driftfix=slew -no-reboot -chardev stdio,id=char0,logfile=serial.log,signal=off -serial chardev:char0

debug:
	qemu-system-i386 -drive format=raw,file=build/boot.iso -boot a -s -S & gdb -ex "target remote localhost:1234"

build/interrupt.o: src/interrupt.asm
	$(AS) -f elf $< -o $@

build/%.o: src/%.c
	$(CC) $(C_FLAGS) -c $< -o $@

build/%.bin: src/%.asm
	$(AS) -f bin $< -o $@

clean:
	rm build/*
