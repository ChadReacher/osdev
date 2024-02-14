#ifndef _DIRENT_H
#define _DIRENT_H

#include <sys/types.h>

// Stat mode constants
#define S_IFMT      0170000
#define S_IFSOCK    014000
#define S_IFLNK     0120000
#define S_IFREG     0100000
#define S_IFBLK     0060000
#define S_IFDIR     0040000
#define S_IFCHR     0020000
#define S_IFIFO     0010000

struct dirent {
	i8 name[256]; 
	u32 inode;
};

typedef struct DIR {
	i32 fd;
	struct dirent dent;
} DIR;

DIR *opendir(const i8 *name);
struct dirent *readdir(DIR *dirp);
void rewinddir(DIR *dirp);
i32 closedir(DIR *dirp);

#endif

