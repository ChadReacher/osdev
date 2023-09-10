TOOLCHAIN_SRC = /usr/bin/i386elfgcc/bin
CC = $(TOOLCHAIN_SRC)/i386-elf-gcc
LD = $(TOOLCHAIN_SRC)/i386-elf-ld
ROOT_DIR = ../..
LIBC = $(ROOT_DIR)/build/libc.a

CFLAGS = -g -c -Wall -m32 -std=c11 -ffreestanding -nostdlib -nostdinc -fno-builtin -nostartfiles -nodefaultlibs -fno-stack-protector -I $(ROOT_DIR)/libc/

default: init

init: init.o
	$(LD) -emain -Ttext 0x0 -o $@ $^ $(LIBC)

init.o: init.c
	$(CC) -o $@ $^ $(CFLAGS)

clean:
	rm -rf init init.o
