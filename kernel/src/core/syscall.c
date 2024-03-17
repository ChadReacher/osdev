#include <syscall.h>
#include <stdio.h>
#include <panic.h>
#include <vfs.h>
#include <fcntl.h>
#include <string.h>
#include <keyboard.h>
#include <screen.h>
#include <process.h>
#include <elf.h>
#include <paging.h>
#include <scheduler.h>
#include <pmm.h>
#include <idt.h>
#include <heap.h>
#include <queue.h>
#include <tss.h>
#include <timer.h>
#include <blk_dev.h>
#include <errno.h>
#include <ext2.h>


extern queue_t *ready_queue;
extern queue_t *procs;
extern u32 startup_time;
extern u32 ticks;
extern queue_t *procs;
extern process_t *init_process;
extern process_t *current_process;
extern void irq_ret();


i32 syscall_open(i8 *filename, u32 oflags, u32 mode) {
	i32 fd, res;
	struct file *f;
	struct ext2_inode *inode;

	mode &= 0777 & ~current_process->umask;
	fd = get_new_fd();
	if (fd > NR_OPEN) {
		debug("We have run out of file descriptors\r\n");
		return -EMFILE;
	}
	current_process->close_on_exec &= ~(1 << fd);
	f = get_empty_file();
	if (!f) {
		return -ENFILE;
	}
	current_process->fds[fd] = f;
	++f->f_count;
	if ((res = open_namei(filename, oflags, mode, &inode)) < 0) {
		current_process->fds[fd] = NULL;
		f->f_count = 0;
		return res;
	}
	f->f_mode = mode;
	f->f_flags = oflags;
	f->f_count = 1;
	f->f_inode = inode;
	f->f_pos = 0;
	return fd;
}

i32 syscall_close(i32 fd) {
	struct file *f;
	if (fd > NR_OPEN) {
		return -EBADF;
	}
	current_process->close_on_exec &= ~(1 << fd);
	f = current_process->fds[fd];
	if (!f) {
		return -EBADF;
	}
	current_process->fds[fd] = NULL;
	if (f->f_count == 0) {
		return 0;
	}
	if (--f->f_count) {
		return 0;
	}
	iput(f->f_inode);
	return 0;
}

i32 syscall_read(i32 fd, i8 *buf, i32 count) {
	struct file *f;
	struct ext2_inode *inode;

	if (fd == FD_STDIN) {
		u8 c = keyboard_getchar();
		if (c) {
			*((u8 *)buf) = c;
			return 1;
		}
		return 0;
	}


	if (count == 0) {
		return 0;
	}
	if (fd > NR_OPEN || !(f = current_process->fds[fd])) {
		return -EBADF;
	}

	inode = f->f_inode;
	if (EXT2_S_ISCHR(inode->i_mode)) {
	}
	if (EXT2_S_ISBLK(inode->i_mode)) {
	}
	if (EXT2_S_ISDIR(inode->i_mode) || EXT2_S_ISREG(inode->i_mode)) {
		if (count + f->f_pos > inode->i_size) {
			count = inode->i_size - f->f_pos;
		}
		if (count <= 0) {
			return 0;
		}
		return ext2_file_read(inode, f, buf, count);
	}
	return -EINVAL;
}

i32 syscall_write(i32 fd, i8 *buf, u32 count) {
	struct file *f;
	struct ext2_inode *inode;

	if (fd == FD_STDOUT || fd == FD_STDERR) {
		u32 i;
		for (i = 0; i < count; ++i) {
			screen_print_char(buf[i]);
		}
		return i;
	}

	if (count == 0) {
		return 0;
	}
	if (fd > NR_OPEN || !(f = current_process->fds[fd])) {
		return -EBADF;
	}

	inode = f->f_inode;
	if (EXT2_S_ISCHR(inode->i_mode)) {
	}
	if (EXT2_S_ISBLK(inode->i_mode)) {
	}
	if (EXT2_S_ISREG(inode->i_mode)) {
		return ext2_file_write(inode, f, buf, count);
	}
	return -EINVAL;
}

#define IS_SEEKABLE(x) ((x)>=1 && (x)<=3)
i32 syscall_lseek(i32 fd, i32 offset, i32 whence) {
	struct file *file;
	i32 tmp;

	if (fd > NR_OPEN || !(file = current_process->fds[fd]) ||
			!(file->f_inode) ||
			!IS_SEEKABLE(MAJOR(file->f_inode->i_dev))) {
		return -EBADF;
	}

	switch (whence) {
		case SEEK_SET:
			if (offset < 0) {
				return -EINVAL;
			}
			file->f_pos = offset;
			break;
		case SEEK_CUR:
			if (file->f_pos + offset < 0) {
				return -EINVAL;
			}
			file->f_pos += offset;
			break;
		case SEEK_END:
			if ((tmp = file->f_inode->i_size + offset) < 0) {
				return -EINVAL;
			}
			file->f_pos = tmp;
			break;
		default:
			return -EINVAL;
	}
	return file->f_pos;
}

i32 syscall_unlink(i8 *filename) {
	const i8 *basename;
	struct buffer *buf;
	i32 err;
	struct ext2_inode *dir, *inode;
	struct ext2_dir *de;

	err = dir_namei(filename, &basename, &dir);
	if (err) {
		return -ENOENT;
	}
	if (*basename == '\0') {
		iput(dir);
		return -ENOENT;
	}
	if (!check_permission(dir, MAY_WRITE)) {
		iput(dir);
		return -EPERM;
	}
	buf = ext2_find_entry(dir, basename, &de, NULL);
	if (!buf) {
		iput(dir);
		return -ENOENT;
	}
	inode = iget(dir->i_dev, de->inode);
	if (!inode) {
		iput(dir);
		free(buf->b_data);
		free(buf);
		return -ENOENT;
	}
	if (!EXT2_S_ISREG(inode->i_mode)) {
		iput(dir);
		iput(inode);
		free(buf->b_data);
		free(buf);
		return -EPERM;
	}
	if (!inode->i_links_count) {
		inode->i_links_count = 1;
	}
	de->inode = 0;
	write_blk(buf);
	--inode->i_links_count;
	inode->i_dirt = 1;
	inode->i_ctime = get_current_time();
	iput(inode);
	iput(dir);
	free(buf->b_data);
	free(buf);
	return 0;
}

i32 syscall_yield() {
	schedule();
	return 0;
}

extern u32 next_pid;
i32 syscall_fork() {
	u32 i;
	process_t *child = malloc(sizeof(process_t));
	if (!child) {
		return -EAGAIN;
	}
	*child = *current_process;
	child->pid = next_pid++;
	child->parent = current_process;
	child->directory = paging_copy_page_dir(1);
	if (!child->directory) {
		--next_pid;
		free(child);
		return -EAGAIN;
	}
	child->kernel_stack_bottom = malloc(4096 *2);
	memset(child->kernel_stack_bottom, 0, 4096 * 2);
	child->regs = (registers_state *)
		(ALIGN_DOWN((u32)child->kernel_stack_bottom + 4096 * 2 - 1, 4)
		 - sizeof(registers_state) + 4);
	*child->regs = *current_process->regs;
	child->context = (context_t *)
		(ALIGN_DOWN((u32)child->kernel_stack_bottom + 4096 * 2 - 1, 4)
		 - sizeof(registers_state) - sizeof(context_t) + 4);
	child->context->eip = (u32)irq_ret;
	child->kernel_stack_top = child->context;
	memcpy(child->fds, current_process->fds, NR_OPEN * sizeof(struct file *));	
	for (i = 0; i < NR_OPEN; ++i) {
		if (child->fds[i]) {
			++child->fds[i]->f_count;
		}
	}
	child->utime = child->stime = child->cutime = child->cstime = 0;
	if (current_process->root) {
		++current_process->root->i_count;
	}
	if (current_process->pwd) {
		++current_process->pwd->i_count;
		child->str_pwd = strdup(current_process->str_pwd);
	}
	child->regs->eax = 0;
	child->alarm = 0;
	child->leader = 0;
	sigemptyset(&child->sigpending);
	queue_enqueue(ready_queue, child);
	queue_enqueue(procs, child);
	return child->pid;
}

i32 syscall_kill(i32 pid, i32 sig) {
	process_t *proc = get_proc_by_id(pid);
	if (proc == NULL) {
		return -1;
	}
	if (sig <= 0 || sig >= NSIG) {
		return -1;
	}

	if (pid > 0) {
		return send_signal(proc, sig);
	} else if (pid == 0) {
		/* TODO: Process group id of sender + permission to send a signal */
		return -1;
	} else if (pid == -1) {
		return 0;
	} else if (pid < 0) {
		/* TODO: Process group id == abs(pid) + have permission to send a signal */
		return -1;
	}
	return 0;
}

i32 syscall_exit(i32 exit_code) {
	u32 i, j;
	void *page_dir_phys;
	page_directory_t *page_dir;
	queue_node_t *node;

	if (current_process->pid == 1) {
		panic("Can't exit the INIT process\r\n");
	}

	/* Free the user code pages in the page directory */
	page_dir_phys = (void *)current_process->directory;
	map_page(page_dir_phys, (void *)0xE0000000, PAGING_FLAG_PRESENT | PAGING_FLAG_WRITEABLE);
	page_dir = (page_directory_t *)0xE0000000;
	for (i = 0; i < 768; ++i) {
		page_directory_entry pde;
		void *table_phys;
		page_table_t *table;

		if (!page_dir->entries[i]) {
			continue;
		}
		pde = page_dir->entries[i];
		table_phys = (void *)GET_FRAME(pde);
		map_page(table_phys, (void *)0xEA000000, PAGING_FLAG_PRESENT | PAGING_FLAG_WRITEABLE);
		table = (page_table_t *)0xEA000000;
		for (j = 0; j < 1024; ++j) {
			page_table_entry pte;
			void *page_frame;
			if (!table->entries[j]) {
				continue;
			}
			pte = table->entries[j];
			page_frame = (void *)GET_FRAME(pte);
			free_blocks(page_frame, 1);
		}
		memset(table, 0, 4096);
		unmap_page((void *)0xEA000000);
		free_blocks(table_phys, 1);
	}
	memset(page_dir, 0, 768 * 4);
	unmap_page((void *)0xE0000000);
	free_blocks(page_dir_phys, 1);
	
	/* Flush TLB */
	__asm__ __volatile__ ("movl %%cr3, %%eax" : : );
	__asm__ __volatile__ ("movl %%eax, %%cr3" : : );
	
	/* Close open files */
	for (i = 3; i < NR_OPEN; ++i) {
		struct file *f = current_process->fds[i];
		(void)f;
		/*
		if (f && f->vfs_node && f->vfs_node != (void *)-1) {
			vfs_close(f->vfs_node);
			free(f->vfs_node);
			memset(f, 0, sizeof(file));
		}
		*/
	}
	/*free(current_process->fds); */
	
	iput(current_process->root);
	current_process->root = NULL;
	iput(current_process->pwd);
	current_process->pwd = NULL;
	free(current_process->str_pwd);

	current_process->exit_code = exit_code;

	/* Pass current_process's children to INIT process */
	node = procs->head;
	for (i = 0; i < procs->len; ++i) {
		process_t *p = (process_t *)node->value;
		if (p->parent == current_process) {
			p->parent = init_process;
		}
		node = node->next;
	}

	syscall_kill(current_process->parent->pid, SIGCHLD);

	current_process->state = ZOMBIE;
	schedule();

	panic("Zombie returned from scheduler\r\n");
	return 0;
}

i32 syscall_waitpid(i32 pid, i32 *stat_loc, i32 options) {
	(void)options;

	if (pid < -1) {
		/* 
		 * Wait for any child process whose process group ID
		 * is equal to the absolute value of 'pid'
		 */
		return 0;
	} else if (pid == -1) {
		/* Wait for any child process */
		for (;;) {
			bool havekids = 0;
			queue_node_t *node = procs->head;
			u32 len = procs->len;
			u32 i;
			for (i = 0; i < len; ++i) {
				u32 chd_pid;
				u32 j;
				queue_node_t *node2;
				process_t *p = (process_t *)node->value;

				if (p->parent != current_process) {
					node = node->next;
					continue;
				}
				havekids = 1;

				if (p->state !=	ZOMBIE && p->state != STOPPED) {
					if (node->next == NULL) {
						break;
					}
					node = node->next;
					continue;
				}
				chd_pid = p->pid;
				if (stat_loc) {
					*stat_loc = p->exit_code & 0xFF;
				}
				/* 
				 * TODO: remove from procs list, get rid of it
				 * and implement in a normal way
				 */
				node2 = procs->head;
				for (j = 0; j < procs->len; ++j) {
					process_t *proc = (process_t *)node2->value;
					if (proc->pid == chd_pid) {
						if (node2->prev == NULL) {
							node2->next->prev = NULL;
							procs->head = node2->next;
							free(node2);
							break;
						} else {
							node2->prev->next = node2->next;
							if (node2->next) {
								node2->next->prev = node2->prev;
							} else {
								procs->tail = node2->prev;
							}
							free(node2);
							break;
						}
					}
					node2 = node2->next;
				}
				--procs->len;
				len = procs->len;

				free(p->kernel_stack_bottom);
				current_process->cutime += p->utime;
				current_process->cstime += p->stime;
				free(p);
				return chd_pid;
			}
			if (!havekids) {
				return -1;
			}
			current_process->state = INTERRUPTIBLE;
			schedule();
		}
	} else if (pid == 0) {
		/*
		 * Wait for any child process whose process group ID is equal
		 * to that of the calling process
		 * at the time of the call to 'waitpid()'
		 */
		return 0;
	} else if (pid > 0) {
		/* Wait for the child whose process ID is equal to the value of 'pid' */
		return 0;
	}
	return -1;
}

i32 syscall_getpid() {
	return current_process->pid;
}

static i32 dup_fd(u32 fd, u32 arg) {
	if (fd > NR_OPEN || !current_process->fds[fd]) {
		return -EBADF;
	}
	if (arg >= NR_OPEN) {
		return -EINVAL;
	}
	while (arg < NR_OPEN) {
		if (current_process->fds[arg]) {
			++arg;
		} else {
			break;
		}
	}
	++(current_process->fds[arg] = current_process->fds[fd])->f_count;
	return arg;
}

i32 syscall_dup2(u32 oldfd, u32 newfd) {
	syscall_close(newfd);
	return dup_fd(oldfd, newfd);
}

i32 syscall_dup(u32 oldfd) {
	return dup_fd(oldfd, 0);
}

i32 syscall_sbrk(i32 incr) {
	u32 old_brk = current_process->brk;

	if (!incr) {
		return old_brk;
	}

	current_process->brk += incr;
	return old_brk;
}

i32 syscall_nanosleep(struct timespec *req, struct timespec *rem) {
	(void)req;
	(void)rem;
	panic("syscall_nanosleep(): UNIMPLEMENTED\n");
	return 0;
}

i32 syscall_getcwd(i8 *buf, u32 size) {
	u32 len;

	if (size <= 0) {
		return -EINVAL;
	}
	len = strlen(current_process->str_pwd);
	if (size > 0 && size < len + 1) {
		return -ERANGE;
	}
	memcpy(buf, current_process->str_pwd, len + 1);
	return 0;
}

i32 syscall_stat(i8 *path, struct stat *statbuf) {
	struct ext2_inode *inode;

	if (!(inode = namei(path))) {
		return -ENOENT;
	}
	statbuf->st_dev = inode->i_dev;
	statbuf->st_ino = inode->i_num;
	statbuf->st_mode = inode->i_mode;
	statbuf->st_nlink = inode->i_links_count;
	statbuf->st_uid = inode->i_uid;
	statbuf->st_gid = inode->i_gid;
	statbuf->st_rdev = 0;
	statbuf->st_size = inode->i_size;
	statbuf->st_atime = inode->i_atime;
	statbuf->st_mtime = inode->i_mtime;
	statbuf->st_ctime = inode->i_ctime;
	statbuf->st_blksize = 1024;
	statbuf->st_blocks = inode->i_blocks;
	return 0;
}

i32 syscall_fstat(i32 fd, struct stat *statbuf) {
	struct file *f;
	struct ext2_inode *inode;

	if (fd >= NR_OPEN || !(f = current_process->fds[fd]) ||
			!(inode = f->f_inode)) {
		return -ENOENT;
	}
	statbuf->st_dev = inode->i_dev;
	statbuf->st_ino = inode->i_num;
	statbuf->st_mode = inode->i_mode;
	statbuf->st_nlink = inode->i_links_count;
	statbuf->st_uid = inode->i_uid;
	statbuf->st_gid = inode->i_gid;
	statbuf->st_rdev = 0;
	statbuf->st_size = inode->i_size;
	statbuf->st_atime = inode->i_atime;
	statbuf->st_mtime = inode->i_mtime;
	statbuf->st_ctime = inode->i_ctime;
	statbuf->st_blksize = 1024;
	statbuf->st_blocks = inode->i_blocks;
	return 0;
}

i32 syscall_chdir(i8 *path) {
	struct ext2_inode *inode;

	if (!(inode = namei(path))) {
		return -ENOENT;
	}
	if (!EXT2_S_ISDIR(inode->i_mode)) {
		iput(inode);
		return -ENOTDIR;
	}
	if (!check_permission(inode, MAY_EXEC)) {
		iput(inode);
		return -EACCES;
	}
	free(current_process->str_pwd);
	iput(current_process->pwd);
	current_process->pwd = inode;
	current_process->str_pwd = strdup(path);
	return 0;
}


i32 syscall_sigaction(i32 sig, sigaction_t *act, sigaction_t *oact, u32 *sigreturn) {
	if (sig < 0 || sig >= NSIG) {
		return -1;
	}
	if (current_process->pid == 1) {
		return -1;
	}

	if (oact) {
		memcpy(oact, &current_process->signals[sig], sizeof(sigaction_t));
	}
	if (act) {
		memcpy(&current_process->signals[sig], act, sizeof(sigaction_t));
		current_process->sigreturn = sigreturn;
		if (act->sa_handler == SIG_DFL 
				&& sigismember(&current_process->sigpending, sig)
				&& sig == SIGCHLD) {
			sigdelset(&current_process->sigpending, sig);
		} else if (act->sa_handler == SIG_IGN
				&& sigismember(&current_process->sigpending, sig)) {
			sigdelset(&current_process->sigpending, sig);
		}
	}
	return 0;
}

i32 syscall_sigprocmask(i32 how, sigset_t *set, sigset_t *oset) {
	if (how != SIG_BLOCK && how != SIG_UNBLOCK && how != SIG_SETMASK) {
		return -1;
	}
	if (current_process->pid == 1) {
		return -1;
	}
	if (oset) {
		memcpy(oset, &current_process->sigmask, sizeof(sigset_t));
	}

	if (set) {
		if (how == SIG_BLOCK) {
			current_process->sigmask |= (*set);
		} else if (how == SIG_UNBLOCK) {
			current_process->sigmask &= (~(*set));
		} else if (how == SIG_SETMASK) {
			current_process->sigmask = *set;
		}
		if (sigismember(&current_process->sigmask, SIGKILL)) {
			sigdelset(&current_process->sigmask, SIGKILL);
		}
		if (sigismember(&current_process->sigmask, SIGSTOP)) {
			sigdelset(&current_process->sigmask, SIGSTOP);
		}
	}
	handle_signal();

	return 0;
}

i32 syscall_sigpending(sigset_t *set) {
	if (!set) {
		return -1;
	}
	memcpy(set, &current_process->sigpending, sizeof(sigset_t));
	return 0;
}

i32 syscall_pause() {
	current_process->state = INTERRUPTIBLE;
	schedule();
	return 0;
}

i32 syscall_sigsuspend(sigset_t *sigmask) {
	sigset_t osigmask;
	if (sigmask == NULL) {
		return -1;
	}
	syscall_sigprocmask(SIG_SETMASK, sigmask, &osigmask);
	syscall_pause();
	syscall_sigprocmask(SIG_SETMASK, &osigmask, NULL);
	return 0;
}


i32 syscall_alarm(u32 secs) {
	if (!secs) {
		return 0;
	}
	current_process->alarm = ticks + secs * TIMER_FREQ;
	return 0;
}

i32 syscall_sleep(u32 secs) {
	(void)secs;
	panic("syscall_sleep(): UNIMPLEMENTED\n");
	return 0;
}

i32 syscall_sigreturn() {
	*current_process->regs = current_process->signal_old_regs;
	memcpy(&current_process->sigmask, &current_process->old_sigmask,sizeof(sigset_t));
	return 0;
}

i32 syscall_getppid() {
	if (current_process->parent) {
		return current_process->parent->pid;
	} else {
		return -1;
	}
}

i32 syscall_getuid() {
	return current_process->uid;
}

i32 syscall_geteuid() {
	return current_process->euid;
}

i32 syscall_getgid() {
	return current_process->gid;
}

i32 syscall_getegid() {
	return current_process->egid;
}

i32 syscall_setuid(u16 uid) {
	if (current_process->euid && current_process->uid) {
		if (uid == current_process->uid) {
			current_process->euid = uid;
		}
		return -1;
	} else {
		current_process->euid = current_process->uid = uid;
	}
	return 0;
}

i32 syscall_setgid(u32 gid) {
	if (current_process->euid && current_process->uid) {
		if (gid == current_process->gid) {
			current_process->egid = gid;
		}
		return -1;
	} else {
		current_process->egid = current_process->gid = gid;
	}
	return 0;
}

i32 syscall_getpgrp() {
	return current_process->pgrp;
}


i32 syscall_setsid() {
	if (current_process->leader) {
		return -1;
	}
	current_process->leader = 1;
	current_process->session = current_process->pgrp = current_process->pid;
	return current_process->pgrp;
}

i32 syscall_setpgid(u32 pid, i32 pgid) {
	u32 i;
	queue_node_t *node;
	if (!pid) {
		pid = current_process->pid;
	}
	if (!pgid) {
		pgid = pid;
	}
	node = procs->head;
	for (i = 0; i < procs->len; ++i) {
		process_t *p = (process_t *)node->value;
		if (p->pid == pid) {
			if (p->leader) {
				return -1;
			}
			if (p->session != current_process->session) {
				return -1;
			}
			p->pgrp = pgid;
		}
		node = node->next;
	}
	return -1;
}

i32 syscall_uname(utsname *name) {
	static utsname thisname = {
		"sierra.0", "nodename", "release ", "version ", "i386    "
	};
	if (!name) {
		return -1;
	}
	memcpy(name, &thisname, sizeof(utsname));
	return 0;
}

i32 syscall_time(i32 *tloc) {
	i32 tm = startup_time + ticks / TIMER_FREQ;
	if (tloc) {
		*tloc = tm;
	}
	return tm;
}

i32 syscall_times(tms *buffer) {
	if (!buffer) {
		return ticks;
	}
	buffer->tms_utime = current_process->utime;
	buffer->tms_stime = current_process->stime;
	buffer->tms_cutime = current_process->cutime;
	buffer->tms_cstime = current_process->cstime;

	return ticks;
}

u32 syscall_umask(u32 cmask) {
	u32 old = current_process->umask;
	current_process->umask = cmask & 0777;
	return old;
}

i32 syscall_link(i8 *path1, i8 *path2) {
	i32 err;
	const i8 *basename;
	struct buffer *buf;
	struct ext2_inode *oldinode, *dir;
	struct ext2_dir *de;

	oldinode = namei(path1);
	if (!oldinode) {
		return -ENOENT;
	}
	if (!EXT2_S_ISREG(oldinode->i_mode)) {
		iput(oldinode);
		return -EPERM;
	}
	err = dir_namei(path2, &basename, &dir);
	if (err) {
		iput(oldinode);
		return -ENOENT;
	}
	if (*basename == '\0') {
		iput(dir);
		iput(oldinode);
		return -EPERM;
	}
	if (dir->i_dev != oldinode->i_dev) {
		iput(dir);
		iput(oldinode);
		return -EXDEV;
	}
	if (!check_permission(dir, MAY_WRITE)) {
		iput(dir);
		iput(oldinode);
		return -EACCES;
	}
	buf = ext2_find_entry(dir, basename, &de, NULL);
	if (buf) {
		free(buf->b_data);
		free(buf);
		iput(dir);
		iput(oldinode);
		return -EEXIST;
	}
	err = ext2_add_entry(dir, basename, &buf, &de);
	if (err) {
		iput(dir);
		iput(oldinode);
		return -err;
	}
	de->inode = oldinode->i_num;
	free(buf->b_data);
	free(buf);
	iput(dir);
	++oldinode->i_links_count;
	oldinode->i_ctime = get_current_time();
	oldinode->i_dirt = 1;
	iput(oldinode);
	return 0;
}

i32 syscall_rename(i8 *oldname, i8 *newname) {
	struct ext2_inode *old_dir, *new_dir;
	const i8 *old_base, *new_base;
	i32 error;

	error = dir_namei(oldname, &old_base, &old_dir);
	if (error) {
		return error;
	}
	if (!check_permission(old_dir, MAY_WRITE | MAY_EXEC)) {
		iput(old_dir);
		return -EACCES;
	}
	if (*old_base == '\0' || (old_base[0] == '.' && strlen(old_base) == 1) ||
			(old_base[1] == '.' && strlen(old_base) == 2)) {
		iput(old_dir);
		return -EPERM;
	}
	error = dir_namei(newname, &new_base, &new_dir);
	if (error) {
		return error;
	}
	if (!check_permission(new_dir, MAY_WRITE | MAY_EXEC)) {
		iput(new_dir);
		iput(old_dir);
		return -EACCES;
	}
	if (*new_base == '\0' || (new_base[0] == '.' && strlen(new_base) == 1) ||
			(new_base[1] == '.' && strlen(new_base) == 2)) {
		iput(new_dir);
		iput(old_dir);
		return -EPERM;
	}
	if (old_dir->i_dev != new_dir->i_dev) {
		iput(new_dir);
		iput(old_dir);
		return -EROFS;
	}
	return ext2_rename(old_dir, old_base, new_dir, new_base);
}

struct dirent *syscall_readdir(DIR *dirp) {
	struct file *file;
	struct ext2_inode *inode;
	i32 i;

	if (dirp->fd >= NR_OPEN ||
			!(file = current_process->fds[dirp->fd]) ||
			!(inode = file->f_inode)) {
		return NULL;
	}
	i = ext2_readdir(inode, file, &dirp->dent);
	if (i) {
		return &dirp->dent;
	} else {
		return NULL;
	}
}

i32 syscall_test(i32 n) {
	i32 i;
	i8 *buf = malloc(8192 * 5);

	debug("START syscall_test()\r\n");
	kprintf("Got n - %d\r\n", n);
	for (i = 0; i < 10000; ++i) {
		i32 fd = syscall_open("/usr/file", O_RDONLY, 0);
		syscall_read(fd, buf, 8192 * 5);
		syscall_close(fd);
	}
	debug("END syscall_test()\r\n");
	return 0;
}

i32 syscall_access(i8 *path, i32 amode) {
	struct ext2_inode *inode;
	i32 res, i_mode;

	amode &= 0007;
	if (!(inode = namei(path))) {
		return -EACCES;
	}
	i_mode = res = inode->i_mode & 0777;
	if (current_process->uid == inode->i_uid) {
		res >>= 6;
	} else if (current_process->gid == inode->i_gid) {
		res >>= 6;
	}
	if ((res & 0007 & amode) == i_mode) {
		return 0;
	}
	if (!current_process->uid &&
			(!(amode & 1) || (i_mode & 0111))) {
		return 0;
	}
	return -EACCES;
}

i32 syscall_chmod(i8 *path, i32 mode) {
	struct ext2_inode *inode;

	if (!(inode = namei(path))) {
		return -ENOENT;
	}
	if ((current_process->euid != inode->i_uid) && current_process->uid != 0) {
		iput(inode);
		return -EACCES;
	}
	inode->i_mode = (mode & 07777) | (inode->i_mode & ~07777);
	inode->i_ctime = get_current_time();
	inode->i_dirt = 1;
	iput(inode);
	return 0;
}

i32 syscall_chown(i8 *path, i32 owner, i32 group) {
	struct ext2_inode *inode;

	if (!(inode = namei(path))) {
		return -ENOENT;
	}
	if (current_process->euid != inode->i_uid && current_process->uid != 0) {
		iput(inode);
		return -EACCES;
	}
	inode->i_uid = owner;
	inode->i_gid = group;
	inode->i_dirt = 1;
	iput(inode);
	return 0;
}

i32 syscall_utime(i8 *path, struct utimbuf *times) {
	struct ext2_inode *inode;
	u32 actime, modtime;

	if (!(inode = namei(path))) {
		return -ENOENT;
	}
	if (times) {
		actime = times->actime;
		modtime = times->modtime;
	} else {
		actime = modtime = get_current_time();
	}
	inode->i_ctime = actime;
	inode->i_mtime = modtime;
	inode->i_dirt = 1;
	iput(inode);
	return 0;
}

i32 syscall_fcntl(i32 fd, i32 cmd, i32 arg) {
	struct file *fp;

	if (fd > NR_OPEN || !(fp = current_process->fds[fd])) {
		return -EBADF;
	}
	switch (cmd) {
		case F_DUPFD:
			return dup_fd(fd, arg);
		case F_GETFD:
			return (current_process->close_on_exec >> fd) & 1;
		case F_SETFD:
			if (arg & 1) {
				current_process->close_on_exec |= (1 << fd);
			} else {
				current_process->close_on_exec &= ~(1 << fd);
			}
			return 0;
		case F_GETFL:
			return fp->f_flags;
		case F_SETFL:
			fp->f_flags &= ~(O_APPEND | O_NONBLOCK);
			fp->f_flags |= arg & (O_APPEND | O_NONBLOCK);
			return 0;
		case F_GETLK:
		case F_SETLK:
		case F_SETLKW:
			return -1;
		default:
			return -1;
	}
	return 0;
}

/* TODO: Make more in-depth check */
static i32 is_empty_dir(struct ext2_inode *inode) {
	return inode->i_size == 24;
}

i32 syscall_rmdir(i8 *path) {
	i32 retval;
	struct buffer *buf;
	const i8 *basename;
	struct ext2_inode *dir, *inode;
	struct ext2_dir *de;

	if ((retval = dir_namei(path, &basename, &dir))) {
		return -ENOENT;
	}
	if (*basename == '\0') {
		iput(dir);
		return -ENOENT;
	}
	if (!check_permission(dir, MAY_WRITE)) {
		iput(dir);
		return -EACCES;
	}
	inode = NULL;
	buf = ext2_find_entry(dir, basename, &de, NULL);
	retval = -ENOENT;
	if (!buf) {
		goto end;
	}
	retval = -EPERM;
	if (!(inode = iget(dir->i_dev, de->inode))) {
		goto end;
	}
	if (inode->i_dev != dir->i_dev) {
		goto end;
	}
	if (inode == dir) {
		goto end;
	}
	if (!EXT2_S_ISDIR(inode->i_mode)) {
		retval = -ENOTDIR;
		goto end;
	}
	if (!is_empty_dir(inode)) {
		retval = -ENOTEMPTY;
		goto end;
	}
	if (inode->i_count > 1) {
		retval = -EBUSY;
		goto end;
	}
	if (inode->i_links_count != 2) {
		debug("inode has links_count > 2, %d\r\n", inode->i_links_count);
	}
	de->inode = 0;
	write_blk(buf);
	inode->i_links_count = 0;
	inode->i_dirt = 1;
	--dir->i_links_count;
	dir->i_ctime = dir->i_mtime = get_current_time();
	dir->i_dirt = 1;
	retval = 0;
end:
	iput(dir);
	iput(inode);
	if (buf) {
		free(buf->b_data);
	}
	free(buf);
	return retval;
}

i32 syscall_mkdir(i8 *path, i32 mode) {
	i32 err;
	struct buffer *buf, *dir_block;
	const i8 *basename;
	struct ext2_inode *dir, *inode;
	struct ext2_dir *de;

	if ((err = dir_namei(path, &basename, &dir))) {
		return -ENOENT;
	}
	if (*basename == '\0') {
		iput(dir);
		return -ENOENT;
	}
	buf = ext2_find_entry(dir, basename, &de, NULL);
	if (buf) {
		free(buf->b_data);
		free(buf);
		iput(dir);
		return -EEXIST;
	}
	inode = alloc_inode(dir->i_dev);
	if (!inode) {
		iput(dir);
		return -ENOSPC;
	}
	inode->i_size = 24;
	inode->i_dirt = 1;
	inode->i_mtime = inode->i_atime = get_current_time();
	if (!(inode->i_block[0] = alloc_block(inode->i_dev))) {
		iput(dir);
		--inode->i_links_count;
		iput(inode);
		return -ENOSPC;
	}
	if (!(dir_block = read_blk(inode->i_dev, inode->i_block[0]))) {
		iput(dir);
		--inode->i_links_count;
		iput(inode);
		return -ERROR;
	}
	de = (struct ext2_dir *)(dir_block->b_data);
	de->inode = inode->i_num;
	de->name_len = 1;
	memcpy(de->name, ".", 1);
	++de;
	de->inode = dir->i_num;
	de->name_len = 2;
	memcpy(de->name, "..", 2);
	inode->i_links_count = 2;
	write_blk(dir_block);
	free(dir_block->b_data);
	free(dir_block);
	inode->i_mode = EXT2_S_IFDIR | (mode & 0777 & ~current_process->umask);
	inode->i_dirt = 1;
	err = ext2_add_entry(dir, basename, &buf, &de);
	if (err) {
		iput(dir);
		inode->i_links_count = 0;
		iput(inode);
		return -ENOSPC;
	}
	de->inode = inode->i_num;
	write_blk(buf);
	++dir->i_links_count;
	dir->i_dirt = 1;
	free(buf->b_data);
	free(buf);
	iput(dir);
	iput(inode);
	return 0;
}
