#ifndef SYSCALL_H
#define SYSCALL_H

#include "types.h"
#include "isr.h"

#define INT_SYSCALL "int $0x80"
#define NB_SYSCALLS  17
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
#define SYSCALL_GETPID	12
#define SYSCALL_DUP		13
#define SYSCALL_SBRK	14
#define SYSCALL_NANOSLEEP 15
#define SYSCALL_GETCWD	16

struct timespec {
	i32 tv_sec;
	i32 tv_nsec;
};

typedef i32 (*syscall_handler_t)(registers_state *regs_state);

void syscall_init();
i32 syscall_handler(registers_state *regs);
void syscall_register_handler(u8 id, syscall_handler_t handler);
i32 syscall_test(registers_state *regs);
i32 syscall_open(registers_state *regs);
i32 syscall_close(registers_state *regs);
i32 syscall_read(registers_state *regs);
i32 syscall_write(registers_state *regs);
i32 syscall_lseek(registers_state *regs);
i32 syscall_unlink(registers_state *regs);
i32 syscall_yield(registers_state *regs);
i32 syscall_exec(registers_state *regs);
i32 syscall_fork(registers_state *regs);
i32 syscall_exit(registers_state *regs);
i32 syscall_waitpid(registers_state *regs);
i32 syscall_getpid(registers_state *regs);
i32 syscall_dup(registers_state *regs);
i32 syscall_sbrk(registers_state *regs);
i32 syscall_nanosleep(registers_state *regs);
i32 syscall_getcwd(registers_state *regs);

#endif
