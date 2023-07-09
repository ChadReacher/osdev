#include "syscall.h"
#include "stdio.h"
#include "panic.h"

syscall_handler_t syscall_handlers[NB_SYSCALLS];

void syscall_init() {
	syscall_register_handler(SYSCALL_TEST, syscall_test);
}

void syscall_handler(registers_state regs) {
	syscall_handler_t handler = syscall_handlers[regs.eax];

	if (handler != 0) {
		handler(regs);
		return;
	}

	PANIC("Received unimplemented syscall: %d\n", regs.eax);
}

void syscall_register_handler(u8 id, syscall_handler_t handler) {
	syscall_handlers[id] = handler;
}

void syscall_test(registers_state regs) {
	kprintf("Hello from syscall_test()\n");
}
