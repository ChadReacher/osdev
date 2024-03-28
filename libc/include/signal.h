#ifndef SIGNAL_H
#define SIGNAL_H

#include <sys/types.h>

typedef u32 sigset_t;

typedef void (*sighandler_t)(i32);

typedef struct {
	sighandler_t sa_handler;
	sigset_t     sa_mask;
	i32          sa_flags;
} sigaction_t;

#define NSIG 32

#define SIGHUP      1
#define SIGINT      2
#define SIGQUIT     3
#define SIGILL      4    
#define SIGTRAP     5
#define SIGABRT     6
#define SIGIOT      6
#define SIGUNUSED   7
#define SIGFPE      8
#define SIGKILL     9
#define SIGUSR1     10
#define SIGSEGV     11
#define SIGUSR2     12
#define SIGPIPE     13
#define SIGALRM     14
#define SIGTERM     15
#define SIGSTKFLT   16
#define SIGCHLD     17
#define SIGCONT     18
#define SIGSTOP     19
#define SIGTSTP     20
#define SIGTTIN     21
#define SIGTTOU     22

#define SIG_ERR ((sighandler_t)-1)
#define SIG_DFL ((sighandler_t)0)
#define SIG_IGN ((sighandler_t)1)

#define SIG_BLOCK   0
#define SIG_UNBLOCK 1
#define SIG_SETMASK 2

#define SA_NOCLDSTOP 0

i32 kill(u32 pid, i32 sig);
i32 sigemptyset(sigset_t *set);
i32 sigfillset(sigset_t *set);
i32 sigaddset(sigset_t *set, i32 signo);
i32 sigdelset(sigset_t *set, i32 signo);
i32 sigismember(const sigset_t *set, i32 signo);
i32 sigaction(i32 sig, sigaction_t *act, sigaction_t *oact);
i32 sigprocmask(i32 how, sigset_t *set, sigset_t *oset);
i32 sigpending(sigset_t *set);
i32 sigsuspend(sigset_t *sigmask);

#endif
