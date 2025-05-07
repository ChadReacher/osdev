; Second stage bootloader. Loads physical memory map, GDT and does other stuff.

bits 16

extern load_kernel

%define CR0_PE 1

%define INT15_FUNC_CODE     0xE820
%define INT15_MAGIC_VALUE   0x534D4150
%define MMAP_ENTRIES_ADDR   0x8500
%define MMAP_ADDR           0x8504

second_stage_start:

	mov byte [drive_num], dl

	; Get physical memory map using BIOS int 0x15 (EAX = 0xE820)
	; At `MMAP_ENTRIES_ADDR` will be stored a number of entries in the memory map
	; At `MMAP_ADDR` will be stored a physical memory map itself
get_memory_map:
	xor bp, bp						; BP will store the number of entries

	; Detect memory ares above 4G
	xor ax, ax
	mov es, ax
	mov di, MMAP_ADDR				; Each list entry is stored at ES:DI [0x0:0x8504]
	mov eax, INT15_FUNC_CODE		; Specific value for int 0x15
	xor ebx, ebx					; Clear EBX as it's a first call
	mov ecx, 24						; Ask for 24 bytes entry
	mov edx, INT15_MAGIC_VALUE		; Magic value, signature
	mov [ES:DI + 20], dword 1		; Force a valid ACPI 3.x entry
	int 0x15
	jc .error						; If carry flag is set, it means "unsupported function"
	cmp eax, INT15_MAGIC_VALUE		; If success, EAX should be set to the magic value
	jne .error
	test ebx, ebx					; Check if EBX = 0? (It implies the list has only 1 entry)
	jz .error						
	jmp .start						; At this point we have a valid entry

.next_entry:
	mov eax, INT15_FUNC_CODE		; Reset EAX
	mov ecx, 24						; Reset ECX
	mov edx, INT15_MAGIC_VALUE		; Magic value
	int 0x15

.start:
	jcxz .skip_entry				; Memory map entry is 0 bytes in length, skip it
	mov ecx, [ES:DI + 8]			; Low 32 bits of length
	or ecx, [ES:DI + 12]			; OR with high 32 bits of length. OR will set the ZF(zero flag)
	jz .skip_entry					; Length of returned memory region is 0, so skip
	inc bp							; Increment the number of entries
	add di, 24

.skip_entry:
	test ebx, ebx					; Check for the end of list? (EBX == 0)
	jz .done
	jmp .next_entry
	
.error:
	stc

.done:
	mov [MMAP_ENTRIES_ADDR], bp		; Store the # of memory map entries
	clc

prepare_for_32:
	call enable_a20

	cli
	
	; Tweak cr0 register to enable A20 line
	mov eax, cr0
	or eax, CR0_PE
	mov cr0, eax
 
	; Should we load IDT?
	lidt [idt]						; Load Interrupt Descriptor Table
	lgdt [gdtp]						; Load bootstrap Global Descriptor Table

	jmp 0x8:entry_32bit				; Far jump to 32 bit mode

bits 32
entry_32bit:
	; Set up segments
	mov ax, 0x10
	mov ds, ax
	mov es, ax
	mov fs, ax
	mov gs, ax
	mov ss, ax

	mov esp, 0x7C00				; Set up the stack pointer
 
	call load_kernel            ; Should never return

spin:
	jmp spin

drive_num: db 0

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
	dw gdt_end - gdt_start - 1				; Limit(size of GDT)
	dd gdt_start							; Base of GDT

idt:
	dw 0
	dd 0
