#ifndef SYSCALL_H
#define SYSCALL_H

#include "types.h"
#include "isr.h"

#define INT_SYSCALL "int $0x80"
#define NB_SYSCALLS  4
#define SYSCALL_TEST 0
#define SYSCALL_READ 1
#define SYSCALL_OPEN 2
#define SYSCALL_CLOSE 3

typedef void (*syscall_handler_t)(registers_state *regs_state);

void syscall_init();
void syscall_handler(registers_state *regs);
void syscall_register_handler(u8 id, syscall_handler_t handler);
void syscall_test(registers_state *regs);
void syscall_open(registers_state *regs);
void syscall_close(registers_state *regs);
void syscall_read(registers_state *regs);

#endif
