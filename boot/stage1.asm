org 0x7C00							; Run the program as if we are loaded into memory 0x7C00
bits 16								; We are in 16 bit mode

boot_start:
	; Setup segment registers
	xor ax, ax
	mov ds, ax
	mov es, ax
	mov fs, ax
	mov gs, ax
	mov ss, ax

	mov sp, 0x9000					; setup stack pointer

	mov [drive_num], dl				; save drive number

	; Load stage2 bootloader at 0x7E00
	mov bx, 0x7E00					; ES:BX => 0x0000:0x7E00 => 0x7E00
	mov ah, 2						; specific value for interrupt int 0x13
	mov al, 6 						; number of sectors to read
	mov ch, 0 						; track/cylinder number
	mov cl, 2 						; sector number (they start at 1, first sector - bootloader)
	mov dh, 0 						; head number
	mov dl, [drive_num] 			; drive number

	int 0x13

	jmp 0x0000:0x7E00				; Jump to second stage bootloader

drive_num: db 0

times 510 - ($ - $$) db 0
dw 0xAA55
