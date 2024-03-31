#ifndef FCNTL_H
#define FCNTL_H

#include "types.h"

#define O_ACCMODE		00003
#define O_RDONLY		   00
#define O_WRONLY		   01
#define O_RDWR			   02
#define O_CREAT			00100
#define O_EXCL   	  	00200
#define O_NOCTTY 	  	00400
#define O_TRUNC			01000
#define O_APPEND		02000
#define O_NONBLOCK		04000

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

#define SEEK_SET 0
#define SEEK_CUR 1
#define SEEK_END 2

u32 open(i8 *filename, u32 oflags, u32 mode);

#endif
