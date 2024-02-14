#include <signal.h>
#include <stdio.h>
#include <unistd.h>

i32 kill(u32 pid, i32 sig) {
	u32 ret;

	__asm__ __volatile__ ("int $0x80" 
			: "=a"(ret) 
			: "a"(__NR_kill), "b"(pid), "c"(sig));

	return ret;
}

i32 sigemptyset(sigset_t *set) {
	if (set == NULL) {
		return -1;
	}
	*set = 0;
	return 0;
}

i32 sigfillset(sigset_t *set) {
	if (set == NULL) {
		return -1;
	}
	*set = ~(sigset_t)0;
	return 0;
}

i32 sigaddset(sigset_t *set, i32 signo) {
	if (signo < 1 || signo > NSIG) {
		return -1;
	}
	*set |= (1 << signo);
	return 0;
}

i32 sigdelset(sigset_t *set, i32 signo) {
	if (signo < 1 || signo > NSIG) {
		return -1;
	}
	*set &= ~(1 << signo);
	return 0;
}

i32 sigismember(const sigset_t *set, i32 signo) {
	if (signo < 1 || signo > NSIG) {
		return -1;
	}
	return (*set) & (1 << signo);
}

static u32 sigreturn() {
	u32 ret;

	__asm__ __volatile__ ("int $0x80" 
			: "=a"(ret) 
			: "a"(__NR_sigreturn));

	return ret;
}

i32 sigaction(i32 sig, sigaction_t *act, sigaction_t *oact) {
	u32 ret;

	__asm__ __volatile__ ("int $0x80" 
			: "=a"(ret) 
			: "a"(__NR_sigaction), "b"(sig), "c"(act), "d"(oact), "S"(sigreturn));

	return ret;
}


i32 sigprocmask(i32 how, sigset_t *set, sigset_t *oset) {
	u32 ret;

	__asm__ __volatile__ ("int $0x80" 
			: "=a"(ret) 
			: "a"(__NR_sigprocmask), "b"(how), "c"(set), "d"(oset));

	return ret;
}

i32 sigpending(sigset_t *set) {
	u32 ret;

	__asm__ __volatile__ ("int $0x80" 
			: "=a"(ret) 
			: "a"(__NR_sigpending), "b"(set));

	return ret;
}

i32 sigsuspend(sigset_t *sigmask) {
	u32 ret;

	__asm__ __volatile__ ("int $0x80" 
			: "=a"(ret) 
			: "a"(__NR_sigsuspend), "b"(sigmask));

	return ret;
}

i32 pause() {
	u32 ret;

	__asm__ __volatile__ ("int $0x80" 
			: "=a"(ret) 
			: "a"(__NR_pause));

	return ret;
}

