global enter_usermode
enter_usermode:
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

global context_switch
context_switch:
	; Restore general-purpose registers
	mov eax, esp
	add eax, 4
	mov ebp, eax		; skip return address
	mov ecx, [ebp + 4]
	mov edx, [ebp + 8]
	mov ebx, [ebp + 12]
	mov esi, [ebp + 24]
	mov edi, [ebp + 28]
	; After that EAX, EBP, ESP are not restored
	; ESP will be restored with 'iret' as user stack

	; Prepare for the usermode switch
	mov ax, 0x23
	mov ds, ax
	mov es, ax
	mov fs, ax
	mov gs, ax

	push 0x23			; User DS
	mov eax, [ebp + 16]
	push eax			; User stack
	push 512			; EFLAGS
	push 0x1B			; User CS
	mov eax, [ebp + 32]
	push eax 			; User EIP

	; Now restore EAX and EBP
	mov eax, [ebp + 0]
	mov ebp, [ebp + 20]

	iret

global switch_to
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
