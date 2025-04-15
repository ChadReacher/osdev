#ifndef FCNTL_H
#define FCNTL_H

#include <sys/types.h>

#define O_RDONLY		   00
#define O_WRONLY		   01
#define O_RDWR			   02
#define O_CREAT			00100
#define O_EXCL   	  	00200
#define O_NOCTTY 	  	00400
#define O_TRUNC			01000
#define O_APPEND		02000
#define O_NONBLOCK		04000

i32 open(const i8 *filename, u32 oflags, u32 mode);
u32 creat(i8 *path, u32 mode);

#endif
