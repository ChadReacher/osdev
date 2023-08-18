#ifndef FD_H
#define FD_H

#include "types.h"

#define FD_STDIN 0
#define FD_STDOUT 1
#define FD_STDERR 2

#define NB_DESCRIPTORS 32

i32 fd_get();
void fd_release(i32 id);

#endif
