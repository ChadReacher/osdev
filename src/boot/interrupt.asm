extern isr_handler
extern irq_handler

%macro def_isr_handler 1
	global isr%1
	isr%1:
		cli
		push %1			; Push an interrupt number
		push %1			; Push a stub error code
		jmp isr_common_stub
%endmacro

%macro def_isr_handler_with_error_code 1
	global isr%1
	isr%1:
		cli
		push %1			; Push an interrupt number
		jmp isr_common_stub
%endmacro

%macro def_irq_handler 1
	global irq%1
	irq%1:
		cli
		push %1			; Push an interrupt number
		push %1 + 32  		; Push a stub error code
		jmp irq_common_stub
%endmacro


isr_common_stub:
	; 1. Save CPU state
	pushad				; Pushes edi, esi, ebp, esp, ebx, edx, ecx, eax
	xor eax, eax
	mov ax, ds			; Lower 16 bits of eax = ds
	push eax			; Save the data segment register

	mov ax, 0x10		; Kernel data segment register
	mov ds, ax
	mov es, ax
	mov fs, ax
	mov gs, ax

	push dword esp			; for "registers_state *regs"

	; 2. Call C handler
	call isr_handler

	; 3. Restore state
	pop eax
	pop eax
	mov ds, ax
	mov es, ax
	mov fs, ax
	mov gs, ax
	popad
	add esp, 8			; Restore stack as we pushed two dwords
	sti
	iret				; Pops 5 things at once: cs, eip, eflags, ss, esp

irq_common_stub:
	; 1. Save CPU state
	pushad
	xor eax, eax
	mov ax, ds
	push eax
	mov ax, 0x10
	mov ds, ax
	mov es, ax
	mov fs, ax
	mov gs, ax

	push dword esp ; for "registers_state *regs"

	; 2. Call C handler
	call irq_handler

	; 3. Restore state
	pop ebx
	pop ebx
	mov ds, bx
	mov es, bx
	mov fs, bx
	mov gs, bx
	popad
	add esp, 8
	sti
	iret

def_isr_handler 0					; Divide Error
def_isr_handler 1   				; Debug exception
def_isr_handler 2   				; Nonmaskable Interrupt
def_isr_handler 3   				; Breakpoint
def_isr_handler 4					; Overflow
def_isr_handler 5   				; BOUND range Exceeded
def_isr_handler 6   				; Invalid Opcode (Undefined Opcode)
def_isr_handler 7   				; Device Not Available (No Math Coprocessor)
def_isr_handler_with_error_code 8   ; Double Fault (with error code - zero)
def_isr_handler 9					; Coprocessor Segment Overrun (reserved)
def_isr_handler_with_error_code 10 	; Invalid TSS (with error code)
def_isr_handler_with_error_code 11	; Segment Not Present (with error code)
def_isr_handler_with_error_code 12	; Stack-Segment Fault (with error code)
def_isr_handler_with_error_code 13	; General Protection (with error code)
def_isr_handler_with_error_code 14	; Page Fault (with error code)
def_isr_handler 15  				; Intel reserved. Do not use
def_isr_handler 16  				; x87 FPU Floating-Point Error (Math Fault)
def_isr_handler_with_error_code 17	; Alignment Check (with error code - zero)
def_isr_handler 18  				; Machine Check
def_isr_handler 19  				; SIMD Floating-Point Exception
def_isr_handler 20  				; Virtualiation Exception
def_isr_handler 21  				; Intel reserved. Do not use
def_isr_handler 22  				; Intel reserved. Do not use 
def_isr_handler 23  				; Intel reserved. Do not use
def_isr_handler 24  				; Intel reserved. Do not use
def_isr_handler 25  				; Intel reserved. Do not use
def_isr_handler 26  				; Intel reserved. Do not use
def_isr_handler 27  				; Intel reserved. Do not use
def_isr_handler 28  				; Intel reserved. Do not use
def_isr_handler 29  				; Intel reserved. Do not use
def_isr_handler 30  				; Intel reserved. Do not use
def_isr_handler 31  				; Intel reserved. Do not use

def_isr_handler 0x80				; Syscall

def_irq_handler 0   ; Programmable Interrupt Timer Interupt
def_irq_handler 1	; Keyboard interrupt
def_irq_handler 2   ; Cascade (used internally by the two PICs. Never raised)
def_irq_handler 3   ; COM2 (if enabled)
def_irq_handler 4   ; COM1 (if enabled)
def_irq_handler 5   ; LPT2 (if enabled)
def_irq_handler 6   ; Floppy Disk
def_irq_handler 7   ; LPT1 / Unreliable "spurious" interrupt (usually)
def_irq_handler 8   ; COMS real-time clock (if enabled)
def_irq_handler 9   ; Free for periphals / legacy SCSI / NIC
def_irq_handler 10  ; Free for periphals / SCSI / NIC
def_irq_handler 11  ; Free for periphals / SCSI / NIC
def_irq_handler 12  ; PS2 Mouse
def_irq_handler 13  ; FPU / Coprocessor / Inter-processor
def_irq_handler 14  ; Primary ATA Hard Disk
def_irq_handler 15  ; Secondary ATA Hard Disk
