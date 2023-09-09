
files = heap.o paging.o pmm.o

.PHONY: mmu
mmu: $(files)

%.o: %.c
	/usr/bin/i386elfgcc/bin/i386-elf-gcc -g -W -Wall -pedantic -m32 -std=c11 -ffreestanding -nostdlib -nostdinc -fno-builtin -nostartfiles -nodefaultlibs -mno-red-zone -fno-stack-protector -I ../../include -I ../../libk/ -c $< -o ../../build/$@
