#ifndef SYSCALL_H
#define SYSCALL_H

#include "types.h"
#include "isr.h"

#define NB_SYSCALLS  2
#define SYSCALL_TEST 1

typedef void (*syscall_handler_t)(registers_state regs_state);

void syscall_init();
void syscall_handler(registers_state regs);
void syscall_register_handler(u8 id, syscall_handler_t handler);
void syscall_test();

#endif
