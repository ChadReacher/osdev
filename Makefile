BIN_UTILS_SRC = /usr/bin/i386elfgcc/bin

CC = $(BIN_UTILS_SRC)/i386-elf-gcc
C_FLAGS = -g -ffreestanding
LD = $(BIN_UTILS_SRC)/i386-elf-ld
AS = nasm

C_SRC = $(wildcard src/*.c)
HEADERS = $(wildcard src/*.h)
ASM_SRC = src/stage1.asm src/stage2.asm src/font.asm

OBJ_SRC = $(C_SRC:src/%.c=bin/%.o)
OBJ_SRC := $(filter-out bin/kernel.o, $(OBJ_SRC))
BIN_SRC = $(ASM_SRC:src/%.asm=bin/%.bin)

all: prepare OS

prepare:
	mkdir -p bin

OS: bin/bootloader.bin bin/kernel.bin
	cat $^ > bin/OS.bin
	dd if=/dev/zero of=./bin/boot.iso bs=512 count=2880
	dd if=./bin/OS.bin of=./bin/boot.iso conv=notrunc bs=512

bin/bootloader.bin: $(BIN_SRC)
	cat $^ > bin/bootloader.bin

bin/kernel.bin: bin/kernel.o $(OBJ_SRC)
	$(LD) -o $@ $^ -Tkernel_linker.ld

run:
	qemu-system-i386 -drive format=raw,file=bin/boot.iso,if=ide,index=0,media=disk

debug:
	qemu-system-i386 -drive format=raw,file=./bin/boot.iso -boot a -s -S & gdb -ex "target remote localhost:1234"

bin/%.o: src/%.c
	$(CC) $(C_FLAGS) -c $< -o $@

bin/%.bin: src/%.asm
	$(AS) -f bin $< -o $@

clean:
	rm -f -r ./bin
