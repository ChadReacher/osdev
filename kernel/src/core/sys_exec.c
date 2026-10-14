#include <syscall.h>
#include <paging.h>
#include <string.h>
#include <errno.h>
#include <isr.h>
#include <process.h>
#include <heap.h>
#include <elf.h>
#include <idt.h>
#include <tss.h>
#include <vfs.h>
#include <panic.h>
#include <binfmt.h>

i32 read_exec_buffer(struct vfs_inode *inode, i8 *buf, i32 bufsz, i32 offset) {
	struct file file;
	i32 err = -ENOEXEC;

	file.f_mode = 1;
	file.f_flags = 0;
	file.f_count = 1;
	file.f_inode = inode;
	file.f_pos = offset;
	file.f_ops = inode->i_f_ops;
	if (!file.f_ops || !file.f_ops->read) {
		goto exit;
	}
	err = file.f_ops->read(file.f_inode, &file, buf, bufsz);
exit:
	return err;
}

static inline i32 count(i8 **ptr) {
	int count = 0;
	for (i8 **curr = ptr; *curr; ++curr) {
		++count;
	}
	return count;
}


#define ARG_MAX 64
i32 syscall_exec(i8 *pathname, i8 **u_argv, i8 **u_envp) {
	i32 err;
	struct binparams bprm;
	i8 **argv, **envp;
	i32 argc = 0, envc = 0;

	err = vfs_open_namei(pathname, 0, 0, &bprm.inode);
	if (err) {
		return err;
	}

	if (!S_ISREG(bprm.inode->i_mode)) {
		err = -EACCES;
		goto err1;
	}
	if (!check_permission(bprm.inode, MAY_READ | MAY_EXEC)) {
		err = -EACCES;
		goto err1;
	}

	memset(bprm.buffer, 0, sizeof(bprm.buffer));
	err = read_exec_buffer(bprm.inode, bprm.buffer, sizeof(bprm.buffer), 0);
	if (err < 0) {
		goto err1;
	}

	argc = count(u_argv);
	envc = count(u_envp);
	if (argc > ARG_MAX || envc > ARG_MAX) {
		err = -E2BIG;
		goto err1;
	}

	argv = (i8 **)malloc((argc + 1) * sizeof(i8 *));
	if (argv == NULL) {
		err = -ENOMEM;
		goto err1;
	}
	envp = (i8 **)malloc((envc + 1) * sizeof(i8 *));
	if (envp == NULL) {
		err = -ENOMEM;
		goto err1;
	}

	for (i32 i = 0; i < argc; ++i) {
		argv[i] = strdup(u_argv[i]);
	}
	argv[argc] = NULL;

	for (i32 i = 0; i < envc; ++i) {
		envp[i] = strdup(u_envp[i]);
	}
	envp[envc] = NULL;

	bprm.argc = argc;
	bprm.argv = argv;
	bprm.envc = envc;
	bprm.envp = envp;

	/* Load a binary */
	err = search_binary_handler(&bprm, current_process->regs);
	if (err != 0) {
		goto err1;
	}

	if (bprm.inode->i_mode & S_ISUID) {
		current_process->euid = bprm.inode->i_uid;
	}
	if (bprm.inode->i_mode & S_ISGID) {
		current_process->egid = bprm.inode->i_gid;
	}
	for (i32 i = 0; i < NSIG; ++i) {
		sighandler_t hand = current_process->signals[i].sa_handler;
		if (hand != SIG_DFL && hand != SIG_IGN && hand != SIG_ERR) {
			current_process->signals[i].sa_handler = SIG_DFL;
		}
	}

err1:
	vfs_iput(bprm.inode);
	return 0;
}
