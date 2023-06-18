org 0x7C00							; Run the program as we loaded into memory 0x7C00
bits 16								; We are in 16 bit mode

boot_start:
	; Setup registers
	xor ax, ax
	mov ds, ax
	mov es, ax
	mov fs, ax
	mov gs, ax
	mov ss, ax

	mov sp, 0x9000					; setup stack pointer

	mov [drive_num], dl				; save drive number

	mov bx, 0x1000
	mov es, bx
	xor bx, bx						; ES:BX => 0x1000:0x0000 => 0x10000

	mov ah, 2						; specific value for interrupt int 0x13
	mov al, 1 						; number of sectors to read
	mov ch, 0 						; track/cylinder number
	mov cl, 2 						; sector number(they start at 1, first sector - bootloader, second - start of kernel)
	mov dh, 0 						; head number
	mov dl, [drive_num] 			; drive number
	
	int 0x13

	; Set video mode to 80x25 16 color text
	mov ah, 0
	mov al, 3
	int 0x10

	call enable_a20

	cli

	; Tweak cr0 register to enable A20 line
	mov eax, cr0
	or eax, 1
	mov cr0, eax

	lidt [idt]						; Load Interrupt Descriptor Table
	lgdt [gdtp]						; Load Global Descriptor Table

	jmp 0x8:entry_32bit				; Far jump to 32 bit mode

bits 32
entry_32bit:
	jmp 0x10000						; Jump to memory where we have loaded the kernel

bits 16
enable_a20:
	cli

	call a20_wait
	mov al, 0xAD
	out 0x64, al

	call a20_wait
	mov al, 0xD0
	out 0x64, al

	call a20_wait2
	in al, 0x60
	push eax

	call a20_wait
	mov al, 0xD1
	out 0x64, al

	call a20_wait
	pop eax
	or al, 2
	out 0x60, al

	call a20_wait
	mov al, 0xAE
	out 0x64, al

	call a20_wait
	sti 
	ret

bits 16
a20_wait:
	in al, 0x64
	test al, 2
	jnz a20_wait
	ret

bits 16
a20_wait2:
	in al, 0x64
	test al, 1
	jz a20_wait2
	ret

gdt_start:
	dq 0
gdt_code_segment:
	dw 0xFFFF
	dw 0
	db 0
	db 0b10011010
	db 0b11001111
	db 0
gdt_data_segment:
	dw 0xFFFF
	dw 0
	db 0
	db 0b10010010
	db 0b11001111
	db 0
gdt_end:

gdtp:
	dw gdt_end - gdt_start - 1					; Limit(size of GDT)
	dd gdt_start								; Base of GDT

idt:
	dw 0
	dd 0

drive_num: db 0

times 510 - ($ - $$) db 0
dw 0xAA55
