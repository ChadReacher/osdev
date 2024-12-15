#ifndef SYS_H
#define SYS_H

#define NR_SYSCALLS 57

typedef i32 (*syscall_fn)();

i32 syscall_test();
i32 syscall_exit();
i32 syscall_fork();
i32 syscall_read();
i32 syscall_write();
i32 syscall_open();
i32 syscall_close();
i32 syscall_waitpid();
i32 syscall_unlink();
i32 syscall_exec();
i32 syscall_chdir();
i32 syscall_time();
i32 syscall_lseek();
i32 syscall_getpid();
i32 syscall_setuid();
i32 syscall_getuid();
i32 syscall_alarm();
i32 syscall_fstat();
i32 syscall_pause();
i32 syscall_kill();
i32 syscall_dup();
i32 syscall_times();
i32 syscall_sbrk();
i32 syscall_setgid();
i32 syscall_getgid();
i32 syscall_geteuid();
i32 syscall_getegid();
i32 syscall_setpgid();
i32 syscall_uname();
i32 syscall_getppid();
i32 syscall_getpgrp();
i32 syscall_setsid();
i32 syscall_sigaction();
i32 syscall_sigpending();
i32 syscall_sigsuspend();
i32 syscall_sigprocmask();
i32 syscall_sigreturn();
i32 syscall_yield();
i32 syscall_getcwd();
i32 syscall_sleep();
i32 syscall_umask();
i32 syscall_link();
i32 syscall_rename();
i32 syscall_readdir();
i32 syscall_stat();
i32 syscall_access();
i32 syscall_dup2();
i32 syscall_fcntl();
i32 syscall_rmdir();
i32 syscall_mkdir();
i32 syscall_getgroups();
i32 syscall_pipe();
i32 syscall_tcsetpgrp();
i32 syscall_tcgetpgrp();
i32 syscall_symlink();
i32 syscall_readlink();
i32 syscall_lstat();

syscall_fn syscall_handlers[NR_SYSCALLS] = {
	syscall_test,
	syscall_exit,
	syscall_fork,
	syscall_read,
	syscall_write,
	syscall_open,
	syscall_close,
	syscall_waitpid,
	syscall_unlink,
	syscall_exec,
	syscall_chdir,
	syscall_time,
	syscall_lseek,
	syscall_getpid,
	syscall_setuid,
	syscall_getuid,
	syscall_alarm,
	syscall_fstat,
	syscall_pause,
	syscall_kill,
	syscall_dup,
	syscall_times,
	syscall_sbrk,
	syscall_setgid,
	syscall_getgid,
	syscall_geteuid,
	syscall_getegid,
	syscall_setpgid,
	syscall_uname,
	syscall_getppid,
	syscall_getpgrp,
	syscall_setsid,
	syscall_sigaction,
	syscall_sigpending,
	syscall_sigsuspend,
	syscall_sigprocmask,
	syscall_sigreturn,
	syscall_yield,
	syscall_getcwd,
	syscall_sleep,
	syscall_umask,
	syscall_link,
	syscall_rename,
	syscall_readdir,
	syscall_stat,
	syscall_access,
	syscall_dup2,
	syscall_fcntl,
	syscall_rmdir,
	syscall_mkdir,
	syscall_getgroups,
	syscall_pipe,
	syscall_tcsetpgrp,
	syscall_tcgetpgrp,
    syscall_symlink,
    syscall_readlink,
	syscall_lstat,
};

#endif
