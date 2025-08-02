%define KERNEL_DS 0x10
%define KERNEL_CS 0x08

global gdt_flush
gdt_flush:
	mov eax, [esp + 4]
	lgdt [eax]

	mov ax, KERNEL_DS
	mov ds, ax
	mov es, ax
	mov fs, ax
	mov gs, ax
	mov ss, ax

	jmp KERNEL_CS:flush

flush:
	ret
