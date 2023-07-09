#include "syscall.h"
#include "stdio.h"

syscall_handler_t syscall_handlers[NB_SYSCALLS];

void syscall_init() {
	register_interrupt_handler(SYSCALL, syscall_handler);

	syscall_register_handler(SYSCALL_TEST, syscall_test);
}

void syscall_handler(registers_state regs) {
	kprintf("SYSCALL!\n"
		  "   Instruction Pointer = 0x%x\n"
		  "   Code Segment        = 0x%x\n"
		  "   CPU Flags           = 0x%x\n"
		  "   Stack Pointer       = 0x%x\n"
		  "   Stack Segment       = 0x%x\n", 
		  regs.eip,
		  regs.cs,
		  regs.eflags,
		  regs.esp,
		  regs.ss
	);
}

void syscall_register_handler(u8 id, syscall_handler_t handler) {
	syscall_handlers[id] = handler;
}

void syscall_test() {
	kprintf("Hello from syscall_test()\n");
}
