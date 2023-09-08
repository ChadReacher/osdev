#ifndef UNISTD_H
#define UNISTD_H

#include "types.h"

#define STDIN_FILENO 0
#define STDOUT_FILENO 1
#define STDERR_FILENO 2

void test(const i8 *);
u32 read(i32, const void *, u32);
u32 write(i32, const void *, u32);
i32 close(i32);
i32 lseek(i32, i32, i32);
i32 unlink(const i8*);
i32 exec(const i8 *);
void yield();
i32 fork();
void exit(i32);
i32 waitpid(i32, i32 *, i32);

#endif
