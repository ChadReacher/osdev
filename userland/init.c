#include <stdio.h>
#include <unistd.h>
#include <stdio.h>
#include <stdlib.h>
#include <signal.h>
#include <sys/utsname.h>
#include <sys/times.h>
#include <sys/wait.h>
#include <time.h>

extern i8 **environ;

void test_sigxset() {
	sigset_t set = 0;

	printf("SIGFILLSET b4: 0x%x, ", set);
	sigfillset(&set);
	printf("after: 0x%x\n", set);

	printf("SIGEMPTYSET b4: 0x%x, ", set);
	sigemptyset(&set);
	printf("after: 0x%x\n", set);

	printf("SIGADDSET SIGFPE b4: 0x%x, ", set);
	sigaddset(&set, SIGFPE);
	printf("after: 0x%x\n", set);

	printf("SIGDELSET SIGFPE b4: 0x%x, ", set);
	sigdelset(&set, SIGFPE);
	printf("after: 0x%x\n", set);

	printf("SIGISMEMBER SIGFPE: %d", sigismember(&set, SIGFPE));
}

void test_sigaction() {
	sigaction_t oact;
	printf("All current sigaction:\n");
	for (int i = 0; i < NSIG; ++i) {
		printf("Call sigaction(%d, NULL, &oact). ", i);
		sigaction(i, NULL, &oact);
		printf("Got the sigaction: "
				"sa_handler: %p, sa_mask: 0x%x, sa_flags: 0x%x\n", 
				oact.sa_handler, oact.sa_mask, oact.sa_flags);
	}
	printf("Change SIGALRM to SIG_IGN\n");
	sigaction_t act;
	act.sa_handler = SIG_IGN;
	sigemptyset(&act.sa_mask);
	act.sa_flags = 0; 
	sigaction(SIGALRM, &act, NULL);

	sigaction(SIGALRM, NULL, &oact);
	printf("Got the sigaction: "
			"sa_handler: %p, sa_mask: 0x%x, sa_flags: 0x%x\n", 
			oact.sa_handler, oact.sa_mask, oact.sa_flags);
}

void test_sigprocmask() {
	sigset_t set, oset, new;

	sigprocmask(SIG_SETMASK, NULL, &oset);
	printf("old signal mask: 0x%x", oset);

	sigemptyset(&set);
	sigaddset(&set, SIGALRM);
	sigprocmask(SIG_BLOCK, &set, &oset);
	sigprocmask(SIG_BLOCK, NULL, &new);
	printf("SIGALRM sigprocmask(SIG_BLOCK). old: 0x%x, new: 0x%x\n",
			oset, new);

	sigemptyset(&set);
	sigaddset(&set, SIGALRM);
	sigprocmask(SIG_UNBLOCK, &set, &oset);
	sigprocmask(SIG_BLOCK, NULL, &new);
	printf("SIGALRM sigprocmask(SIG_UNBLOCK). old: 0x%x, new: 0x%x\n",
			oset, new);

	sigemptyset(&set);
	sigaddset(&set, SIGALRM);
	sigaddset(&set, SIGFPE);
	sigprocmask(SIG_SETMASK, &set, &oset);
	sigprocmask(SIG_BLOCK, NULL, &new);
	printf("SIGALRM, SIGFPE sigprocmask(SIG_SETMASK). old: 0x%x, new: 0x%x\n",
			oset, new);

	sigemptyset(&set);
	sigaddset(&set, SIGKILL);
	sigaddset(&set, SIGSTOP);
	sigprocmask(SIG_SETMASK, &set, &oset);
	sigprocmask(SIG_BLOCK, NULL, &new);
	printf("SIGKILL, SIGSTOP sigprocmask(SIG_SETMASK). old: 0x%x, new: 0x%x\n",
			oset, new);

	sigemptyset(&set);
	sigaddset(&set, SIGKILL);
	sigaddset(&set, SIGSTOP);
	sigprocmask(SIG_BLOCK, &set, &oset);
	sigprocmask(SIG_BLOCK, NULL, &new);
	printf("SIGKILL, SIGSTOP sigprocmask(SIG_BLOCK). old: 0x%x, new: 0x%x\n",
			oset, new);
}

void test_sigsuspend() {
	sigset_t sigmask;

	sigemptyset(&sigmask);
	sigaddset(&sigmask, SIGFPE);
	sigsuspend(&sigmask);
	printf("After sigsuspend\n");
}

void signal_fpe_handler(i32 signo) {
	printf("got signal: %d, ppid: %d\n", signo, getppid());
	printf("Got FPE, be careful, my friend.\n");
	printf("My uid: %d, euid: %d, gid: %d, egid: %d, pgrp: %d\n",
			getuid(), geteuid(), getgid(), getegid(), getpgrp());
	printf("Create new session");
	setsid();
	printf("Create new session with new pgrp: %d\n", getpgrp());
}

int main() {
	/*
	int x = 10;
	int y = 20;
	utsname name;
	uname(&name);
	printf("sysname - %s, nodename - %s, release - %s, version - %s, machine - %s\n",
			name.sysname, name.nodename, name.release, name.version, name.machine);
	i8 pid = fork();
	if (pid == 0) {
		printf("x - %d\n", x);
		printf("y - %d\n", y);
		sigaction_t act;
		act.sa_handler = signal_fpe_handler;
		sigemptyset(&act.sa_mask);
		act.sa_flags = 0; 
		sigaction(SIGFPE, &act, NULL);
		test_sigxset();
		test_sigaction();
		test_sigprocmask();
		test_sigsuspend();
		int x = 1 / 0;
		for (int i = 0; i < 100; ++i) {
			printf("sky child: %d\n", i);
			if (i == 50) {
				kill(getpid(), SIGFPE);
			}
		}
	} else {
		i32 stat_loc;
		i32 ch_pid = wait(&stat_loc);
		printf("child-pid - %d, exit-code: %d\n", ch_pid, stat_loc);
		printf("ppid of init: %d\n", getppid());
		i32 tloc;
		time(&tloc);
		printf("seconds since Epoch: %d\n", tloc);
		tms buffer;
		times(&buffer);
		printf("utime: %d, stime: %d, cutime: %d, cstime: %d\n",
				buffer.tms_utime, buffer.tms_stime,
				buffer.tms_cutime, buffer.tms_cstime);
		for (int i = 0; i < 10; ++i) {
			printf("sky parent: %d\n", i);
		}

	}
	*/
	
	i32 pid = fork();
	if (pid == 0) {
		i8 *m[] = {"/bin/sh", 0};
		execv("/bin/sh", m);
		printf("After exec\n");
	}
	i32 stat_loc = 0;
	do {
		pid = wait(&stat_loc);
		printf("PID of terminated child - %d\n", pid);
		printf("Return value of terminated child - %d\n", stat_loc);
	} while (pid != -1);
	printf("PID of last terminated child - %d\n", pid);
	printf("Return value of last terminated child - %d\n", stat_loc);
	return 0;
}
