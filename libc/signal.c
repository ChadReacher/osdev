#include "signal.h"
#include "stdio.h"

i32 kill(u32 pid, i32 sig) {
	u32 ret;

	__asm__ __volatile__ ("int $0x80" 
			: "=a"(ret) 
			: "a"(19), "b"(pid), "c"(sig));

	return ret;
}

u32 sigreturn() {
	u32 ret;

	__asm__ __volatile__ ("int $0x80" 
			: "=a"(ret) 
			: "a"(27));

	return ret;
}

i32 sigaction(i32 sig, sigaction_t *act, sigaction_t *oact) {
	u32 ret;

	printf("kill: 0x%x\n", kill);
	printf("sigreturn: 0x%x\n", sigreturn);
	__asm__ __volatile__ ("int $0x80" 
			: "=a"(ret) 
			: "a"(20), "b"(sig), "c"(act), "d"(oact), "S"(sigreturn));

	return ret;
}


i32 sigprocmask(i32 how, sigset_t *set, sigset_t *oset) {
	u32 ret;

	__asm__ __volatile__ ("int $0x80" 
			: "=a"(ret) 
			: "a"(21), "b"(how), "c"(set), "d"(oset));

	return ret;
}

i32 sigpending(sigset_t *set) {
	u32 ret;

	__asm__ __volatile__ ("int $0x80" 
			: "=a"(ret) 
			: "a"(22), "b"(set));

	return ret;
}

i32 sigsuspend(sigset_t *sigmask) {
	u32 ret;

	__asm__ __volatile__ ("int $0x80" 
			: "=a"(ret) 
			: "a"(23), "b"(sigmask));

	return ret;
}

i32 pause() {
	u32 ret;

	__asm__ __volatile__ ("int $0x80" 
			: "=a"(ret) 
			: "a"(24));

	return ret;
}

