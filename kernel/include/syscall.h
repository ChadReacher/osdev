#ifndef SYSCALL_H
#define SYSCALL_H

#include "types.h"

#define S_IFMT   0170000
#define S_IFSOCK 014000
#define S_IFLNK  0120000
#define S_IFREG  0100000
#define S_IFBLK  0060000
#define S_IFDIR  0040000
#define S_IFCHR  0020000
#define S_IFIFO  0010000

#define F_DUPFD 0
#define F_GETFD 1
#define F_SETFD 2
#define F_GETFL 3
#define F_SETFL 4
#define F_GETLK 5
#define F_SETLK 6
#define F_SETLKW 7

#define WNOHANG   1
#define WUNTRACED 2

#define WIFEXITED(s)	(((s) & 0xFF) == 0)
#define WEXITSTATUS(s)	(((s) >> 8) & 0xFF)
#define WIFSIGNALED(s)	((((unsigned int)(s) - 1) & 0xFFFF) < 0xFF)
#define WTERMSIG(s)		((s >> 8) & 0xFF)
#define WIFSTOPPED(s)	(((s) & 0xFF) == 0x7F)
#define WSTOPSIG(s)		(((s) >> 8) & 0xFF)

struct dirent {
	i8 name[256]; 
	u32 inode;
};

typedef struct DIR {
	i32 fd;
	struct dirent dent;
} DIR;

struct utimbuf {
	u32 actime;
	u32 modtime;
};

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


#endif
