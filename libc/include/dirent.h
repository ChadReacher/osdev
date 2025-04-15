#ifndef _DIRENT_H
#define _DIRENT_H

#include <sys/types.h>

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

