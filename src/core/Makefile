
files = elf.o idt.o isr.o kshell.o panic.o syscall.o gdt.o tss.o

.PHONY: core
core: $(files)

%.o: %.c
	/usr/bin/i386elfgcc/bin/i386-elf-gcc -g -W -Wall -pedantic -m32 -std=c11 -ffreestanding -nostdlib -nostdinc -fno-builtin -nostartfiles -nodefaultlibs -mno-red-zone -fno-stack-protector -I ../../include -I ../../libk/ -c $< -o ../../build/$@
