#ifndef SYSCALL_H
#define SYSCALL_H

#include "types.h"
#include "isr.h"

#define NR_SYSTEM_CALLS 41

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

typedef i32 (*syscall_fn)();

void syscall_init();
i32 syscall_handler(registers_state *regs);

#endif
