#ifndef UNISTD_H
#define UNISTD_H

#include "sys/types.h"

#define _POSIX_SOURCE 1
// #define _POSIX_JOB_CONTROL
// #define _POSIX_SAVED_IDS
#define _POSIX_VERSION 198808L

#define _POSIX_CHOWN_RESTRICTED
// #define _POSIX_NO_TRUNC
#define _POSIX_VDISABLE '\0'

#define F_OK        0 // Exists?
#define X_OK        1 // Execute?
#define R_OK        2 // Read?
#define W_OK        4 // Write?

#define SEEK_SET   0
#define SEEK_CUR   1
#define SEEK_END   2

#define STDIN_FILENO 0
#define STDOUT_FILENO 1
#define STDERR_FILENO 2

extern i8 **environ;

struct timespec {
	i32 tv_sec;
	i32 tv_nsec;
};

void test(const i8 *);
u32 read(i32, const void *, u32);
u32 write(i32, const void *, u32);
i32 close(i32);
i32 lseek(i32, i32, i32);
i32 unlink(const i8*);
i32 execvpe(const i8 *, i8 *const [], i8 *const []);
i32 execv(const i8 *, i8 *const []);
void yield();
i32 fork();
void _exit(i32);
i32 getpid();
i32 dup(i32);
void *sbrk(u32);
i32 nanosleep(const struct timespec *, struct timespec *);
u32 sleep(u32);
i8 *getcwd(i8 *, u32);
i32 chdir(const i8 *);
i32 access(const i8 *pathname, i32 mode);
i8 *getenv(const i8 *);
u32 alarm(u32 secs);
i32 getppid();
u16 getuid();
u16 geteuid();
u8 getgid();
u8 getegid();
i32 getpgrp();
i32 setsid();
i32 setpgid(i32 pid, i32 pgid);

#endif
