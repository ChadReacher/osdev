#ifndef _UNISTD_H
#define _UNISTD_H

#include "sys/types.h"

#define _POSIX_SOURCE 1
/* #define _POSIX_JOB_CONTROL */
/* #define _POSIX_SAVED_IDS */
#define _POSIX_VERSION 198808L

#define _POSIX_CHOWN_RESTRICTED 1
/* #define _POSIX_NO_TRUNC */
#define _POSIX_VDISABLE '\0'

#define F_OK        0
#define X_OK        1
#define R_OK        2
#define W_OK        4


#define STDIN_FILENO 0
#define STDOUT_FILENO 1
#define STDERR_FILENO 2


#define __NR_test			0
#define __NR_exit           1
#define __NR_fork			2
#define __NR_read			3
#define __NR_write			4
#define __NR_open			5
#define __NR_close			6
#define __NR_waitpid		7
#define __NR_unlink			8
#define __NR_execve			9
#define __NR_chdir			10
#define __NR_time			11
#define __NR_lseek			12
#define __NR_getpid			13
#define __NR_setuid			14
#define __NR_getuid			15
#define __NR_alarm			16
#define __NR_fstat			17
#define __NR_pause			18
#define __NR_kill			19
#define __NR_dup			20
#define __NR_times			21
#define __NR_sbrk			22
#define __NR_setgid			23
#define __NR_getgid			24
#define __NR_geteuid		25
#define __NR_getegid		26
#define __NR_setpgid		27
#define __NR_uname			28
#define __NR_getppid		29
#define __NR_getpgrp		30
#define __NR_setsid			31
#define __NR_sigaction		32
#define __NR_sigpending		33
#define __NR_sigsuspend		34
#define __NR_sigprocmask	35
#define __NR_sigreturn		36
#define __NR_yield			37
#define __NR_getcwd			38
#define __NR_sleep			39
#define __NR_umask          40
#define __NR_link           41
#define __NR_rename         42
#define __NR_readdir        43
#define __NR_stat			44
#define __NR_access			45
#define __NR_dup2			46
#define __NR_fcntl			47
#define __NR_rmdir			48
#define __NR_mkdir          49
#define __NR_getgroups		50
#define __NR_pipe			51
#define __NR_tcsetpgrp		52
#define __NR_tcgetpgrp		53
#define __NR_symlink		54
#define __NR_readlink		55
#define __NR_lstat			56
#define __NR_truncate       57
#define __NR_mount          58

#define syscall0(type, name) \
type name(void) { \
	type ret; \
	__asm__ __volatile__ ("int $0x80" \
			: "=a"(ret) \
			: "a"(__NR_##name)); \
	if (ret >= 0) { \
		return ret; \
	} \
	errno = -ret; \
	return -1; \
}

#define syscall1(type, name, atype, a) \
type name(atype a) { \
	type ret; \
	__asm__ __volatile__ ("int $0x80" \
			: "=a"(ret) \
			: "a"(__NR_##name), "b"(a)); \
	if (ret >= 0) { \
		return ret; \
	} \
	errno = -ret; \
	return -1; \
}

#define syscall2(type, name, atype, a, btype, b) \
type name(atype a, btype b) { \
	type ret; \
	__asm__ __volatile__ ("int $0x80" \
			: "=a"(ret) \
			: "a"(__NR_##name), "b"(a), "c"(b)); \
	if (ret >= 0) { \
		return ret; \
	} \
	errno = -ret; \
	return -1; \
}

#define syscall3(type, name, atype, a, btype, b, ctype, c) \
type name(atype a, btype b, ctype c) { \
	type ret; \
	__asm__ __volatile__ ("int $0x80" \
			: "=a"(ret) \
			: "a"(__NR_##name), "b"(a), "c"(b), "d"(c)); \
	if (ret >= 0) { \
		return ret; \
	} \
	errno = -ret; \
	return -1; \
}

#define syscall4(type, name, atype, a, btype, b, ctype, c, dtype, d) \
type name(atype a, btype b, ctype c, dtype d) { \
	type ret; \
	__asm__ __volatile__ ("int $0x80" \
			: "=a"(ret) \
			: "a"(__NR_##name), "b"(a), "c"(b), "d"(c), "S"(d)); \
	if (ret >= 0) { \
		return ret; \
	} \
	errno = -ret; \
	return -1; \
}

struct timespec {
	i32 tv_sec;
	i32 tv_nsec;
};

pid_t fork();
i32 execve(const i8 *path, i8 **argv, i8 **envp);
i32 execv(const i8 *path, i8 **argv);
i32 execvp(const i8 *file, i8 **argv);
i32 execl(const i8 *path, const i8 *arg, ...);
i32 execlp(const i8 *file, const i8 *arg, ...);
i32 execle(const i8 *path, const i8 *arg, ...);

void test(i32);
u32 read(i32, const void *, u32);
u32 write(i32, const void *, u32);
i32 close(i32);
i32 lseek(i32, i32, i32);
i32 unlink(const i8*);
void yield();
void _exit(i32);
i32 getpid();
i32 dup(i32);
void *sbrk(u32);
u32 sleep(u32);
i8 *getcwd(i8 *, u32);
i32 chdir(const i8 *);

#define SEEK_SET   0
#define SEEK_CUR   1
#define SEEK_END   2

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
i32 link(i8 *path1, i8 *path2);
i32 symlink(i8 *path1, i8 *path2);
i32 readlink(const i8 *pathname, i8 *buf, i32 bufsiz);
i32 rename(i8 *old, i8 *new);
i32 access(i8 *path, i32 amode);
i32 dup2(u32 oldfd, u32 newfd);
i32 fcntl(i32 fd, i32 cmd, i32 arg);
i32 mkdir(i8 *path, i32 mode);
i32 rmdir(i8 *path);
void perror(const i8 *s);
i32 getgroups(i32 gidsetsize, i32 *grouplist);
i32 pipe(i32 fildes[2]);
i32 tcsetpgrp(i32 fildes, pid_t pgrp_id);
i32 tcgetpgrp(i32 fildes);
i32 truncate(const char *path, u32 length);

#endif
