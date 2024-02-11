#ifndef UNISTD_H
#define UNISTD_H

#include "types.h"
#include "stat.h"

#define STDIN_FILENO 0
#define STDOUT_FILENO 1
#define STDERR_FILENO 2

#define F_OK        0 // Exists?
#define X_OK        0 // Execute?
#define R_OK        0 // Read?
#define W_OK        0 // Write?

extern i8 **environ;

struct timespec {
	i32 tv_sec;
	i32 tv_nsec;
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
void exit(i32);
i32 waitpid(i32, i32 *, i32);
i32 wait(i32 *);
i32 getpid();
i32 dup(i32);
void *sbrk(u32);
i32 nanosleep(const struct timespec *, struct timespec *);
u32 sleep(u32);
i8 *getcwd(i8 *, u32);
i32 stat(const i8 *, struct stat *);
i32 fstat(i32, struct stat *);
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
i32 uname(utsname *name);
i32 time(i32 *tloc);
i32 times(tms *buffer);

#endif
