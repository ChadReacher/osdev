#include <syscall.h>
#include <ext2.h>
#include <paging.h>
#include <string.h>
#include <errno.h>
#include <isr.h>
#include <process.h>
#include <heap.h>
#include <elf.h>
#include <idt.h>
#include <tss.h>

extern process_t *current_process;

i32 syscall_close(i32 fd);

#define ARG_MAX 64
i32 syscall_exec(i8 *pathname, i8 **u_argv, i8 **u_envp) {
	struct ext2_inode *inode;
	i32 err, i;
	i8 **argv, **envp, **arg_p, **env_p;
	i32 argc = 0, envc = 0;

	err = namei(pathname, &inode);
	if (err) {
		return err;
	} else if (!EXT2_S_ISREG(inode->i_mode)) {
		iput(inode);
		return -EACCES;
	} else if (!check_permission(inode, MAY_READ | MAY_EXEC)) {
		iput(inode);
		return -EACCES;
	}


	if (u_argv) {
		for (arg_p = u_argv; *arg_p; ++arg_p) {
			++argc;
		}
	}
	if (u_envp) {
		for (env_p = u_envp; *env_p; ++env_p) {
			++envc;
		}
	}
	if (argc > ARG_MAX || envc > ARG_MAX) {
		iput(inode);
		return -E2BIG;
	}
	argv = (i8 **)malloc((argc + 1) * sizeof(i8 *));
	envp = (i8 **)malloc((envc + 1) * sizeof(i8 *));
	argv[argc] = NULL;
	envp[envc] = NULL;
	for (i = 0; i < argc; ++i) {
		argv[i] = strdup(u_argv[i]);
	}
	for (i = 0; i < envc; ++i) {
		envp[i] = strdup(u_envp[i]);
	}



	if ((err = elf_load(inode, argc, argv, envc, envp))) {
		for (i = 0; i < argc; ++i) {
			free(argv[i]);
		}
		for (i = 0; i < envc; ++i) {
			free(envp[i]);
		}
		free(argv);
		free(envp);
		iput(inode);
		return err;
	}
	if (inode->i_mode & EXT2_S_ISUID) {
		current_process->euid = inode->i_uid;
	}
	if (inode->i_mode & EXT2_S_ISGID) {
		current_process->egid = inode->i_gid;
	}
	for (i = 0; i < NSIG; ++i) {
		sighandler_t hand = current_process->signals[i].sa_handler;
		if (hand != SIG_DFL && hand != SIG_IGN && hand != SIG_ERR) {
			current_process->signals[i].sa_handler = SIG_DFL;
		}
	}
	for (i = 0; i < NR_OPEN; ++i) {
		if ((current_process->close_on_exec >> i) & 1) {
			syscall_close(i);
		}
	}
	current_process->close_on_exec = 0;
	iput(inode);

	tss_set_stack((u32)current_process->kernel_stack_top);
	return 0;
}
