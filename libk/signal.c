#include <signal.h>

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
