#ifndef WAIT_H
#define WAIT_H

#include <sys/types.h>

extern i32 waitpid(pid_t pid, i32 *stat_loc, i32 options);
extern i32 wait(i32 *stat_loc);

#endif
