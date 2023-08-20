
#files = build/elf.o build/idt.o build/isr.o build/kshell.o build/panic.o build/syscall.o
files = elf.o idt.o isr.o kshell.o panic.o syscall.o

.PHONY: core
core: $(files)

%.o: %.c
	/usr/bin/i386elfgcc/bin/i386-elf-gcc -g -W -Wall -pedantic -m32 -std=c11 -ffreestanding -nostdlib -nostdinc -fno-builtin -nostartfiles -nodefaultlibs -mno-red-zone -fno-stack-protector -I ../../include -I ../../libc/ -c $< -o ../../build/$@
