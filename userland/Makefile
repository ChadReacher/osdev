TOOLCHAIN_SRC = /usr/bin/i386elfgcc/bin
CC = $(TOOLCHAIN_SRC)/i386-elf-gcc
LD = $(TOOLCHAIN_SRC)/i386-elf-ld
ROOT_DIR = ..
LIBC = $(ROOT_DIR)/build/libc.a

CFLAGS = -g -c -W -Wall -pedantic -m32 -std=c11 -ffreestanding -nostdlib -nostdinc -fno-builtin -nostartfiles -nodefaultlibs -mno-red-zone\
	 -fno-stack-protector -I $(ROOT_DIR)/libc/

files = bin/init bin/test bin/sh bin/ls bin/cat bin/rm

default: userland
.PHONY: userland
userland: ../build/crt0.o $(files)

../build/crt0.o: crt0.S
	nasm -f elf32 -I ./libc/ $< -o $@

bin/%: $(ROOT_DIR)/build/%.o 
	$(LD) -Ttext 0x0 -o $@ $(ROOT_DIR)/build/crt0.o $^ $(LIBC)

$(ROOT_DIR)/build/%.o: %.c
	$(CC) $(CFLAGS) $< -o $@
