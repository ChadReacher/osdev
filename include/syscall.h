#ifndef SYSCALL_H
#define SYSCALL_H

#include "types.h"
#include "isr.h"

#define INT_SYSCALL "int $0x80"
#define NB_SYSCALLS  41
#define SYSCALL_TEST	      0
#define SYSCALL_READ	      1
#define SYSCALL_WRITE	      2
#define SYSCALL_OPEN	      3
#define SYSCALL_CLOSE	      4
#define SYSCALL_LSEEK	      5
#define SYSCALL_UNLINK	      6
#define SYSCALL_YIELD	      7
#define SYSCALL_EXEC	      8
#define SYSCALL_FORK	      9
#define SYSCALL_EXIT	      10
#define SYSCALL_WAITPID	      11
#define SYSCALL_GETPID	      12
#define SYSCALL_DUP		      13
#define SYSCALL_SBRK	      14
#define SYSCALL_NANOSLEEP     15
#define SYSCALL_GETCWD	      16
#define SYSCALL_FSTAT	      17
#define SYSCALL_CHDIR         18
#define SYSCALL_KILL          19
#define SYSCALL_SIGACTION     20
#define SYSCALL_SIGPROCMASK   21
#define SYSCALL_SIGPENDING    22
#define SYSCALL_SIGSUSPEND    23
#define SYSCALL_PAUSE         24
#define SYSCALL_ALARM         25
#define SYSCALL_SLEEP         26
#define SYSCALL_SIGRETURN     27
#define SYSCALL_GETPPID       28
#define SYSCALL_GETUID        29
#define SYSCALL_GETEUID       30
#define SYSCALL_GETGID        31
#define SYSCALL_GETEGID       32
#define SYSCALL_SETUID        33
#define SYSCALL_SETGID        34
#define SYSCALL_GETPGRP       35
#define SYSCALL_SETSID        36
#define SYSCALL_SETPGID       37
#define SYSCALL_UNAME         38
#define SYSCALL_TIME          39
#define SYSCALL_TIMES         40

#define S_IFMT 0170000
#define S_IFSOCK 014000
#define S_IFLNK 0120000
#define S_IFREG 0100000
#define S_IFBLK 0060000
#define S_IFDIR 0040000
#define S_IFCHR 0020000
#define S_IFIFO 0010000

struct timespec {
	i32 tv_sec;
	i32 tv_nsec;
};

struct stat {
	u32 st_dev;
	u32 st_ino;
	u32 st_mode;
	u32 st_nlink;
	u32 st_uid;
	u32 st_gid;
	u32 st_rdev;
	u32 st_size;
	u32 st_atime;
	u32 st_mtime;
	u32 st_ctime;
	u32 st_blksize;
	u32 st_blocks;
};

typedef struct {
	i8 sysname[9];
	i8 nodename[9];
	i8 release[9];
	i8 version[9];
	i8 machine[9];
} utsname;

typedef struct {
	i32 tms_utime;
	i32 tms_stime;
	i32 tms_cutime;
	i32 tms_cstime;
} tms;

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
i32 syscall_fstat(registers_state *regs);
i32 syscall_chdir(registers_state *regs);
i32 syscall_kill(registers_state *regs);
i32 syscall_sigaction(registers_state *regs);
i32 syscall_sigprocmask(registers_state *regs);
i32 syscall_sigpending(registers_state *regs);
i32 syscall_sigsuspend(registers_state *regs);
i32 syscall_pause(registers_state *regs);
i32 syscall_alarm(registers_state *regs);
i32 syscall_sleep(registers_state *regs);
i32 syscall_sigreturn(registers_state *regs);
i32 syscall_getppid(registers_state *regs);
i32 syscall_getuid(registers_state *regs);
i32 syscall_geteuid(registers_state *regs);
i32 syscall_getgid(registers_state *regs);
i32 syscall_getegid(registers_state *regs);
i32 syscall_setuid(registers_state *regs);
i32 syscall_setgid(registers_state *regs);
i32 syscall_getpgrp(registers_state *regs);
i32 syscall_setsid(registers_state *regs);
i32 syscall_setpgid(registers_state *regs);
i32 syscall_uname(registers_state *regs);
i32 syscall_time(registers_state *regs);
i32 syscall_times(registers_state *regs);

#endif
