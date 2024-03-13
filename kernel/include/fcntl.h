#ifndef FCNTL_H
#define FCNTL_H

#include "types.h"

#define O_ACCMODE 0x0003
#define O_RDONLY  0x0000
#define O_WRONLY  0x0001
#define O_RDWR	  0x0002

#define O_CREAT    00100
#define O_EXCL     00200
#define O_NOCTTY   00400
#define O_TRUNC    01000
#define O_APPEND   02000
#define O_NONBLOCK 04000


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

// lseek 'whence' symbolic constants
#define SEEK_SET 1
#define SEEK_CUR 2
#define SEEK_END 3

u32 open(i8 *filename, u32 oflags, u32 mode);

#endif
