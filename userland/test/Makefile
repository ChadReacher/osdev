TOOLCHAIN_SRC = /usr/bin/i386elfgcc/bin
CC = $(TOOLCHAIN_SRC)/i386-elf-gcc
LD = $(TOOLCHAIN_SRC)/i386-elf-ld
ROOT_DIR = ../..
LIBC = $(ROOT_DIR)/build/libc.a

CFLAGS = -g -c -W -Wall -pedantic -m32 -std=c11 -ffreestanding -nostdlib -nostdinc -fno-builtin -nostartfiles -nodefaultlibs -mno-red-zone\
	 -fno-stack-protector -I $(ROOT_DIR)/libc/

default: test

test: $(ROOT_DIR)/build/test.o
	$(LD) -Ttext 0x0 -o ../bin/$@ $(ROOT_DIR)/build/crt0.o $^ $(LIBC)

$(ROOT_DIR)/build/test.o: test.c
	$(CC) $(CFLAGS) $< -o $@

clean:
	rm -rf test test.o
