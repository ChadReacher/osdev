#ifndef WAIT_H
#define WAIT_H

#include <sys/types.h>

#define WNOHANG   1
#define WUNTRACED 2

/*
 * Internal representation of status is following:
 * [1 info byte] [1 code byte]
 *
 * If @code is 0x0, then child has exited. @info is exit code.
 * If @code is 0x1 - 0x7E, then child has exited due to a signal. @info is signal number.
 * If @code is 0x1F, then child has stopped. @info byte is signal number that caused the stop.
 */
#define WIFEXITED(s)	(((s) & 0xFF) == 0x00)
#define WEXITSTATUS(s)	(((s) >> 8) & 0xFF)

#define WIFSIGNALED(s)	(((s) & 0xFF) == 0x7E)
#define WTERMSIG(s)		(((s) >> 8) & 0xFF)

#define WIFSTOPPED(s)	(((s) & 0xFF) == 0x7F)
#define WSTOPSIG(s)		(((s) >> 8) & 0xFF)

pid_t waitpid(pid_t pid, i32 *stat_loc, i32 options);
pid_t wait(i32 *stat_loc);

#endif
