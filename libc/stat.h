#ifndef STAT_H
#define STAT_H

#include <types.h>

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


#endif
