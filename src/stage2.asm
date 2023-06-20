; Second stage bootloader. Loads VBE, GDT and other stuff.
org 0x7E00
bits 16

second_stage_start:
	; Get VESA BIOS information
	xor ax, ax
	mov es, ax
	mov ah, 0x4F
	mov di, vbe_info_block			; Set up the pointer at memory location where we want to save information
	int 0x10

	cmp ax, 0x4F					; 0x4F is a success, everything else - error
	jne error

	mov ax, word [vbe_info_block.video_mode_pointer]
	mov [offset], ax
	mov ax, word [vbe_info_block.video_mode_pointer + 2]
	mov [t_segment], ax

	mov fs, ax
	mov si, [offset]

.find_mode:
	mov dx, [fs:si]
	inc si
	inc si
	mov [offset], si
	mov [mode], dx

	cmp dx, word 0xFFFF				; Are we at the end of the mode list?
	je end_of_modes

	; Get VESA mode infomration
	mov ax, 0x4F01
	mov cx, [mode]
	mov di, mode_info_block
	int 0x10

	cmp ax, 0x4F
	jne error	

	; Find mode that matches our width, height and bpp(bits per pixel)
	mov ax, [width]
	cmp ax, [mode_info_block.x_resolution]
	jne .next_mode

	mov ax, [height]
	cmp ax, [mode_info_block.y_resolution]
	jne .next_mode

	mov ax, [bpp]
	cmp al, [mode_info_block.bits_per_pixel]
	jne .next_mode
	
	; We have found a mode, let's set this VBE mode
	mov ax, 0x4F02
	mov bx, [mode]
	or bx, 0x4000					; Enable linear frame buffer(bit 14)
	int 0x10

	cmp ax, 0x4F
	jne error

	jmp prepare_for_32

.next_mode:
	mov ax, [t_segment]
	mov fs, ax
	mov si, [offset]
	jmp .find_mode

prepare_for_32:
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
	; Set up segments
	mov ax, 0x10
	mov ds, ax
	mov es, ax
	mov fs, ax
	mov gs, ax
	mov ss, ax
	mov esp, 0x90000				; Set up the stack pointer

	; Load VBE mode info block at memory location 0x9000
	mov esi, mode_info_block
	mov edi, 0x9000
	mov ecx, 64
	rep movsd

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

bits 16
error:
	mov ax, 0x0E45			; print 'E'
	int 0x10
	cli
	hlt

bits 16
end_of_modes:
	mov ax, 0x0E4E			; print 'N'
	int 0x10
	cli
	hlt
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

error_msg: db "Error occurred with VBE configuring", 0

; VBE variables
width:		dw 1920
height:  	dw 1080
bpp:	 	db 32
offset:  	dw 0
t_segment: 	dw 0
mode:	 	dw 0

times 512 - ($ - $$) db 0

; Sector 2
vbe_info_block:
	.vbe_signature: db "VBE2"
	.vbe_version: dw 0
	.oem_string_pointer: dd 0
	.capabilities: dd 0
	.video_mode_pointer: dd 0
	.total_memory: dw 0
	.oem_software_rev: dw 0
	.oem_vendor_name_pointer: dd 0
	.eom_product_name_pointer: dd 0
	.oem_product_revision_pointer: dd 0
	.reserved: times 222 db 0
	.oem_data: times 256 db 0

; Sector 3
mode_info_block:
	; Mandatory info for all VBE revisions
	.mode_attributes: dw 0
	.window_a_attributes: db 0
	.window_b_attributes: db 0
	.window_granularity: dw 0
	.window_size: dw 0
	.window_a_segment: dw 0
	.window_b_segment: dw 0
	.window_function_pointer: dd 0
	.bytes_per_scanline: dw 0

	; Mandatory info for VBE 1.2 and above
	.x_resolution: dw 0
	.y_resolution: dw 0
	.x_charsize: db 0
	.y_charsize: db 0
	.number_of_planes: db 0
	.bits_per_pixel: db 0
	.number_of_banks: db 0
	.memory_model: db 0
	.bank_size: db 0
	.number_of_image_pages: db 0
	.reserved1: db 1

    ;; Direct color fields (required for direct/6 and YUV/7 memory models)
	.red_mask_size: db 0
	.red_field_position: db 0
	.green_mask_size: db 0
	.green_field_position: db 0
	.blue_mask_size: db 0
	.blue_field_position: db 0
	.reserved_mask_size: db 0
	.reserved_field_position: db 0
	.direct_color_mode_info: db 0

	; Mandatory info for VBE 2.0 and above
	.physical_base_pointer: dd 0     ; Physical address for flat memory frame buffer
	.reserved2: dd 0
	.reserved3: dw 0

	; Mandatory info for VBE 3.0 and above
	.linear_bytes_per_scan_line: dw 0
	.bank_number_of_image_pages: db 0
	.linear_number_of_image_pages: db 0
	.linear_red_mask_size: db 0
	.linear_red_field_position: db 0
	.linear_green_mask_size: db 0
	.linear_green_field_position: db 0
	.linear_blue_mask_size: db 0
	.linear_blue_field_position: db 0
	.linear_reserved_mask_size: db 0
	.linear_reserved_field_position: db 0
	.max_pixel_clock: dd 0

	.reserved4: times 190 db 0      ; Remainder of mode info block

; Sector padding
times 1536-($-$$) db 0
