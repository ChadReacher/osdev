global jump_usermode
jump_usermode:
	cli
	xor eax, eax
	mov ax, 0x23
	mov ds, ax
	mov es, ax
	mov fs, ax
	mov gs, ax

	push 0x23
	push esp
	pushf
	push 0x1B
	push user_test
	iret

user_test:
	mov eax, 0x123
	jmp $
	
