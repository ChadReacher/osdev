#ifndef FCNTL_H
#define FCNTL_H

#include <sys/types.h>

// Open a file for reading only
#define O_RDONLY 0x0000
// Open a file for writing only
#define O_WRONLY 0x0001
// Open a file for reading and writing
#define O_RDWR	 0x0002
// Start writing at the end of a file
#define O_APPEND 0x0008
// Create, then open a file
#define O_CREAT  0x0100
// Truncate a file
#define O_TRUNC  0x0200

// Symbolic constants for 'mode'
#define S_IRWXU 0x0700
#define S_IRUSR 0x0400
#define S_IWUSR 0x0200
#define S_IXUSR 0x0100

#define S_IRWXG 0x0070
#define S_IRGRP 0x0040
#define S_IWGRP 0x0020
#define S_IXGRP 0x0010

#define S_IRWXO 0x0007
#define S_IROTH 0x0004
#define S_IWOTH 0x0002
#define S_IXOTH 0x0001


u32 open(const i8 *filename, u32 oflags, u32 mode);

#endif
