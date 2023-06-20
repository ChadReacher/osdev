# Comments start with '#'
BIN_SRC=/usr/bin/i386elfgcc/bin

CC=$(BIN_SRC)/i386-elf-gcc
AS=nasm
LD=$(BIN_SRC)/i386-elf-ld

all: prepare OS

prepare:
	mkdir -p bin

bin/kernel.bin: bin/kernel.o bin/screen.o
	$(LD) -o $@ $^ -Tkernel_linker.ld

bin/kernel.o: src/kernel.c
	$(CC) -ffreestanding -g -c $< -o $@

bin/screen.o: src/screen.c
	$(CC) -ffreestanding -c $< -o $@

bin/font.bin: src/font.asm
	$(AS) -f bin $< -o $@

bin/stage1.bin: src/stage1.asm
	$(AS) -f bin $< -o $@

bin/stage2.bin: src/stage2.asm
	$(AS) -f bin $< -o $@

OS: bin/stage1.bin bin/stage2.bin bin/font.bin bin/kernel.bin
	cat bin/stage1.bin bin/stage2.bin bin/font.bin bin/kernel.bin > bin/OS.bin
	dd if=/dev/zero of=./bin/boot.iso bs=512 count=2880
	dd if=./bin/OS.bin of=./bin/boot.iso conv=notrunc bs=512

run:
	qemu-system-i386 -drive format=raw,file=bin/boot.iso,if=ide,index=0,media=disk

debug:
	qemu-system-i386 -drive format=raw,file=./bin/boot.iso -boot a -s -S & gdb -ex "target remote localhost:1234"

clean:
	rm -f -r ./bin
