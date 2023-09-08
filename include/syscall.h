#ifndef SYSCALL_H
#define SYSCALL_H

#include "types.h"
#include "isr.h"

#define INT_SYSCALL "int $0x80"
#define NB_SYSCALLS  12
#define SYSCALL_TEST	0
#define SYSCALL_READ	1
#define SYSCALL_WRITE	2
#define SYSCALL_OPEN	3
#define SYSCALL_CLOSE	4
#define SYSCALL_LSEEK	5
#define SYSCALL_UNLINK	6
#define SYSCALL_YIELD	7
#define SYSCALL_EXEC	8
#define SYSCALL_FORK	9
#define SYSCALL_EXIT	10
#define SYSCALL_WAITPID	11

typedef void (*syscall_handler_t)(registers_state *regs_state);

void syscall_init();
void syscall_handler(registers_state *regs);
void syscall_register_handler(u8 id, syscall_handler_t handler);
void syscall_test(registers_state *regs);
void syscall_open(registers_state *regs);
void syscall_close(registers_state *regs);
void syscall_read(registers_state *regs);
void syscall_write(registers_state *regs);
void syscall_lseek(registers_state *regs);
void syscall_unlink(registers_state *regs);
void syscall_yield(registers_state *regs);
void syscall_exec(registers_state *regs);
void syscall_fork(registers_state *regs);
void syscall_exit(registers_state *regs);
void syscall_waitpid(registers_state *regs);

#endif
