TOOLCHAIN_SRC = /usr/bin/i386elfgcc/bin
CC = $(TOOLCHAIN_SRC)/i386-elf-gcc
LD = $(TOOLCHAIN_SRC)/i386-elf-ld
LIBK = ../build/libk.a

CFLAGS = -c -Wall -m32 -std=c11 -O2 -ffreestanding -nostdlib -fno-builtin -fno-stack-protector

default: init

init: init.o
	$(LD) -emain -o $@ $^ $(LIBK)

init.o: init.c
	$(CC) -o $@ $^ $(CFLAGS)

clean:
	rm -rf $(TARGET) *.o
