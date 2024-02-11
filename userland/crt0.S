extern environ
extern _exit
extern main

global _start
_start:
	pop eax				; argc
	pop ebx				; argv
	pop ecx				; envp

	; Set up end of stack frame
	push 0				; return address
	push 0				; frame pointer
	mov ebp, esp

	mov [environ], ecx

	push ecx
	push ebx
	push eax

	call main

	; Terminate the process with an exit code
	push eax
	call _exit
