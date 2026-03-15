global enter_usermode_asm
enter_usermode_asm:
	mov ebx, [esp + 4] ; user stack
	mov eax, 0x23
	mov ds, ax
	mov es, ax
	mov fs, ax
	mov gs, ax
	push eax			; user SS
	push ebx			; user stack
	push 0x200			; EFLAGS
	push 0x1B			; user CS
	push 0x0			; EIP
	iret

global switch_to
; switch_to(struct context **old_context, struct context *new_context);
; eax -> **old_context
; edx -> *new_context
switch_to:
	mov eax, [esp + 4]
	mov edx, [esp + 8]

	push ebp
	push ebx
	push esi
	push edi

	mov [eax], esp
	mov esp, edx

	pop edi
	pop esi
	pop ebx
	pop ebp

	ret
