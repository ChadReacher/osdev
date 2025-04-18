#include <syscall.h>
#include <stdio.h>
#include <panic.h>
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
#include <tss.h>
#include <timer.h>
#include <blk_dev.h>
#include <errno.h>
#include <ext2.h>
#include <pipe.h>
#include <chr_dev.h>
#include <tty.h>
#include <vfs.h>
#include <bcache.h>


extern u32 startup_time;
extern u32 ticks;
extern struct proc *procs[NR_PROCS];
extern u32 next_pid;
extern struct file file_table[NR_FILE];
extern struct tty_struct tty_table[];

extern void irq_ret();

i32 syscall_open(i8 *filename, u32 oflags, u32 mode) {
	i32 fd, res;
	struct file *f;
	struct vfs_inode *inode;

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
	if ((res = vfs_open_namei(filename, oflags, mode, &inode)) < 0) {
		current_process->fds[fd] = NULL;
		f->f_count = 0;
		return res;
	}
	f->f_mode = mode;
	f->f_flags = oflags;
	f->f_count = 1;
	f->f_inode = inode;
	f->f_pos = 0;
    f->f_ops = inode->i_f_ops;
	if (f->f_ops && f->f_ops->open) {
		res = f->f_ops->open(inode, f);
		if (res != 0) {
			vfs_iput(inode);
			current_process->fds[fd] = NULL;
			--f->f_count;
			return res;
		}
	}
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
	vfs_iput(f->f_inode);
	return 0;
}

i32 syscall_read(i32 fd, i8 *buf, i32 count) {
	struct file *f;
	struct vfs_inode *inode;

	if (count == 0) {
		return 0;
	}
	if (fd > NR_OPEN || !(f = current_process->fds[fd])) {
		return -EBADF;
	}
	inode = f->f_inode;
	if (f->f_ops && f->f_ops->read) {
		return f->f_ops->read(inode, f, buf, count);
	}
	return -EINVAL;
}

i32 syscall_write(i32 fd, i8 *buf, u32 count) {
	struct file *f;
	struct vfs_inode *inode;

	if (count == 0) {
		return 0;
	}
	if (fd > NR_OPEN || !(f = current_process->fds[fd])) {
		return -EBADF;
	}

	inode = f->f_inode;
	if (f->f_ops && f->f_ops->write) {
		return f->f_ops->write(inode, f, buf, count);
	}
	return -EINVAL;
}

#define IS_SEEKABLE(x) ((x)>=1 && (x)<=3)
i32 syscall_lseek(i32 fd, i32 offset, i32 whence) {
	struct file *file;
	i32 tmp;

	if (fd > NR_OPEN || !(file = current_process->fds[fd]) ||
			!(file->f_inode) ||
			!IS_SEEKABLE(MAJOR(file->f_inode->i_rdev))) {
		return -EBADF;
	}
	if (file->f_inode->i_pipe) {
		return -ESPIPE;
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
	i32 err;
	struct vfs_inode *dir;

	err = vfs_dirnamei(filename, NULL, &basename, &dir);
	if (err) {
		return err;
	}
	if (*basename == '\0') {
		vfs_iput(dir);
		return -ENOENT;
	}
	if (!check_permission(dir, MAY_WRITE)) {
		vfs_iput(dir);
		return -EPERM;
	}
	if (!dir->i_ops || !dir->i_ops->unlink) {
		vfs_iput(dir);
		return -ENOENT;
	}
    return dir->i_ops->unlink(dir, basename);
}

i32 syscall_yield() {
	schedule();
	return 0;
}

i32 syscall_fork() {
	u32 i;
	i32 idx;
	struct proc *child;

	idx = get_free_proc();
	if (idx < 0) { 
		panic("No more procs\r\n");
		return -ENOMEM;
	}
	child = procs[idx] = malloc(sizeof(struct proc));
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
	child->regs = (struct registers_state *)
		(ALIGN_DOWN((u32)child->kernel_stack_bottom + 4096 * 2 - 1, 4)
		 - sizeof(struct registers_state) + 4);
	*child->regs = *current_process->regs;
	child->context = (struct context *)
		(ALIGN_DOWN((u32)child->kernel_stack_bottom + 4096 * 2 - 1, 4)
		 - sizeof(struct registers_state) - sizeof(struct context) + 4);
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
	return child->pid;
}

i32 kill_pgrp(i32 pgrp, i32 sig) {
	i32 i, err, retval = -ESRCH;
	i32 found = 0;
	struct proc *p;

	if (sig < 0 || sig > 32 || pgrp <= 0) {
		return -EINVAL;
	}
	for (i = 0; i < NR_PROCS; ++i) {
		p = procs[i];
		if (!p) {
			continue;
		}
		if (p->pgrp == pgrp) {
			if (sig && (err = send_signal(p, sig))) {
				retval = err;
			} else {
				++found;
			}
		}
	}
	return (found ? 0 : retval);
}

i32 syscall_kill(i32 pid, i32 sig) {
	struct proc *p;

	if (pid == 0) {
		return kill_pgrp(current_process->pgrp, sig);
	} else if (pid == -1) {
		debug("syscall_kill(pid %d, sig %d) - behaviour unspecified", pid, sig);
		return 0;
	} else if (pid < 0) {
		return kill_pgrp(-pid, sig);
	}
	p = get_proc_by_id(pid);
	if (!p) {
		return -ESRCH;
	}
	if (sig <= 0 || sig >= NSIG) {
		return -EINVAL;
	}
	return send_signal(p, sig);
}

i32 is_orphaned_pgrp(i32 pgrp) {
	i32 i;
	struct proc *p;

	for (i = 0; i < NR_PROCS; ++i) {
		p = procs[i];
		if (!p) {
			continue;
		}
		if (p->pgrp != pgrp ||
				p->state == ZOMBIE ||
				p->parent->pid == 1) {
			continue;
		}
		if (p->parent->pgrp != pgrp &&
				p->parent->session == p->session) {
			return 0;
		}
	}
	return 1;
}

static i32 has_stopped_jobs(i32 pgrp) {
	i32 i;
	struct proc *p;

	for (i = 0; i < NR_PROCS; ++i) {
		p = procs[i];
		if (!p) { 
			continue;
		}
		if (p->pgrp != pgrp) {
			continue;
		}
		if (p->state == STOPPED) {
			return 1;
		}
	}
	return 0;
}

void do_exit(i32 code) {
	u32 i;

	/* Close open files */
	for (i = 0; i < NR_OPEN; ++i) {
		if (current_process->fds[i]) {
			syscall_close(i);
		}
	}
	vfs_iput(current_process->root);
	current_process->root = NULL;
	vfs_iput(current_process->pwd);
	current_process->pwd = NULL;

	if (current_process->pid == 1) {
		sync_inodes();
		sync_buffers();
		panic("Can't exit the INIT process\r\n");
	}
	free_user_image();
        free_blocks((void *)current_process->directory, 1);

	if (current_process->leader && current_process->tty >= 0) {
		tty_table[current_process->tty].pgrp = 0;
	}
	free(current_process->str_pwd);
	free(current_process->kernel_stack_bottom);
	current_process->state = ZOMBIE;
	current_process->exit_code = code;
	if (current_process->parent->pgrp != current_process->pgrp &&
			current_process->parent->session == current_process->session && 
			is_orphaned_pgrp(current_process->pgrp) &&
			has_stopped_jobs(current_process->pgrp)) {
		kill_pgrp(current_process->pgrp, SIGHUP);
		kill_pgrp(current_process->pgrp, SIGCONT);
	}
	syscall_kill(current_process->parent->pid, SIGCHLD);

	/* Pass current_process's children to INIT process */
	for (i = 0; i < NR_PROCS; ++i) {
		struct proc *p = procs[i];
		if (!p) {
			continue;
		}
		if (p->parent == current_process) {
			p->parent = procs[1];
			if (p->state == ZOMBIE) {
				(procs[1])->sigpending |= (1 << (SIGCHLD - 1));
			}
			if (p->pgrp != current_process->pgrp &&
					p->session == current_process->session && 
					is_orphaned_pgrp(p->pgrp) &&
					has_stopped_jobs(p->pgrp)) {
				kill_pgrp(p->pgrp, SIGHUP);
				kill_pgrp(p->pgrp, SIGCONT);
			}	
		}
	}

	schedule();

	panic("Zombie returned from scheduler\n");
}

void syscall_exit(i32 exit_code) {
	do_exit((exit_code & 0xFF) << 8);
}

i32 syscall_waitpid(i32 pid, i32 *stat_loc, i32 options) {
	i32 i, flag;
	struct proc *p;
	sigset_t oldsigmask;

loop:
	flag = 0;
	for (i = 0; i < NR_PROCS; ++i) {
		p = procs[i];
		if (!p) { 
			continue;
		}
		if (p->parent != current_process) {
			continue;
		}
		if (pid > 0) {
			if (p->pid != pid) {
				continue;
			}
		} else if (!pid) {
			if (p->pgrp != current_process->pgrp) {
				continue;
			}
		} else if (pid != -1) {
			if (p->pgrp != -pid) {
				continue;
			}
		}
		switch (p->state) {
			case STOPPED:
				if (!(options & WUNTRACED) || 
						!p->exit_code) {
					continue;
				}
				if (stat_loc) {
					*stat_loc = (p->exit_code << 8) | 0x7F;
				}
				p->exit_code = 0;
				return p->pid;
			case ZOMBIE:
				current_process->cutime += p->utime;
				current_process->cstime += p->stime;
				flag = p->pid;
				if (stat_loc) {
					*stat_loc = p->exit_code;
				}
				procs[i] = NULL;
				return flag;
			default:
				flag = 1;
				continue;
		}
	}
	if (flag) {
		if (options & WNOHANG) {
			return 0;
		}
		current_process->state = INTERRUPTIBLE;
		oldsigmask = current_process->sigmask;
		sigdelset(&current_process->sigmask, SIGCHLD);
		schedule();
		current_process->sigmask = oldsigmask;
		if (current_process->sigpending & 
				~(current_process->sigmask | (1 << (SIGCHLD)))) {
			debug("waitpid: ERESTART\n");
			return -ERESTART;
		}
		goto loop;
	}
	return -ECHILD;
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

// TODO[fs]: implement proper getcwd() using bottom-up approach via iget and namei
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
	i32 err;
	struct vfs_inode *inode;

	if ((err = vfs_namei(path, NULL, 1, &inode))) {
		return err;
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
	statbuf->st_blksize = inode->i_sb->s_block_size;
	statbuf->st_blocks = inode->i_blocks;
	vfs_iput(inode);
	return 0;
}

i32 syscall_fstat(i32 fd, struct stat *statbuf) {
	struct file *f;
	struct vfs_inode *inode;

	if (fd >= NR_OPEN || !(f = current_process->fds[fd]) ||
			!(inode = f->f_inode)) {
		return -EBADF;
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
	statbuf->st_blksize = inode->i_sb->s_block_size;
	statbuf->st_blocks = inode->i_blocks;
	vfs_iput(inode);
	return 0;
}

// TODO[fs]: remove str_pwd after implementing proper getcwd()
i32 syscall_chdir(i8 *path) {
	i32 err;
	struct vfs_inode *inode;

	if ((err = vfs_namei(path, NULL, 1, &inode))) {
		return err;
	}
	if (!S_ISDIR(inode->i_mode)) {
		vfs_iput(inode);
		return -ENOTDIR;
	}
	if (!check_permission(inode, MAY_EXEC)) {
		vfs_iput(inode);
		return -EACCES;
	}
	free(current_process->str_pwd);
	vfs_iput(current_process->pwd);
	current_process->pwd = inode;
	current_process->str_pwd = strdup(path);
	return 0;
}

i32 syscall_sigaction(i32 sig, sigaction_t *act, sigaction_t *oact, u32 *sigreturn) {
	if (sig <= 0 || sig >= NSIG || sig == SIGKILL || sig == SIGSTOP) {
		return -EINVAL;
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
		} else {
			return -EINVAL;
		}
		if (sigismember(&current_process->sigmask, SIGKILL)) {
			sigdelset(&current_process->sigmask, SIGKILL);
		}
		if (sigismember(&current_process->sigmask, SIGSTOP)) {
			sigdelset(&current_process->sigmask, SIGSTOP);
		}
	}
	return 0;
}

i32 syscall_sigpending(sigset_t *set) {
	sigset_t tmp;
	if (!set) {
		return -1;
	}
	tmp = current_process->sigpending & current_process->sigmask;
	memcpy(set, &tmp, sizeof(sigset_t));
	return 0;
}

i32 syscall_pause() {
	current_process->state = INTERRUPTIBLE;
	schedule();
	return -EINTR;
}

i32 syscall_sigsuspend(sigset_t *sigmask) {
	sigset_t osigmask = current_process->sigmask;
	if (sigmask == NULL) {
		return -1;
	}
	current_process->sigmask = *sigmask;
	syscall_pause();
	current_process->sigmask = osigmask;
	return -EINTR;
}


i32 syscall_alarm(u32 secs) {
	i32 old = current_process->alarm;
	if (old) {
		old = (old - ticks) / TIMER_FREQ;
	}
	current_process->alarm = secs > 0 ? ticks + secs * TIMER_FREQ : 0;
	return 0;
}

i32 syscall_sleep(u32 secs) {
	current_process->state = INTERRUPTIBLE;
	current_process->sleep = ticks + secs * TIMER_FREQ;
	schedule();
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

i32 syscall_getgroups(i32 gidsetsize, i32 *grouplist) {
	i32 i;
	for (i = 0; i < NR_GROUPS && current_process->groups[i] != -1;
			++i, ++grouplist) {
		if (gidsetsize) {
			if (i >= gidsetsize) { 
				return -EINVAL;
			}
			*grouplist = current_process->groups[i];
		}
	}
	return i;
}

i8 *getlogin() {
	debug("getlogin(): unimplemented\r\n");
	return NULL;
}

i8 *cuserid(i8 *s) {
	debug("cuserid(%d): unimplemented\r\n", s);
	return NULL;
}

i32 syscall_getpgrp() {
	return current_process->pgrp;
}


i32 syscall_setsid() {
	if (current_process->leader) {
		return -EPERM;
	}
	current_process->leader = 1;
	current_process->session = current_process->pgrp = current_process->pid;
	current_process->tty = -1;
	return current_process->pgrp;
}

static i32 session_of_pgrp(i32 pgrp) {
	u32 i;
	for (i = 0; i < NR_PROCS; ++i) {
		struct proc *p = procs[i];
		if (!p) {
			continue;
		}
		if (p->pgrp == pgrp) {
			return p->session;
		}
	}
	return -1;
}

i32 syscall_setpgid(i32 pid, i32 pgid) {
	u32 i;
	if (!pid) {
		pid = current_process->pid;
	}
	if (!pgid) {
		pgid = pid;
	}
	if (pgid < 0) {
		return -EINVAL;
	}
	for (i = 0; i < NR_PROCS; ++i) {
		struct proc *p = procs[i];
		if (!p) {
			continue;
		}
		if (p->pid == pid && 
				(p->parent == current_process || p == current_process)) {
			if (p->leader) {
				return -EPERM;
			}
			if (p->session != current_process->session ||
					(pgid != pid &&
					 session_of_pgrp(pgid) != current_process->session)) {
				return -EPERM;
			}
			p->pgrp = pgid;
			return 0;
		}
	}
	return -ESRCH;
}

i32 syscall_uname(utsname *name) {
	static utsname thisname = {
		"sierra.0", "nodename", "release ", "version ", "i386    "
	};
	if (!name) {
		return -EINVAL;
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
	struct vfs_inode *inode, *dir;

	err = vfs_namei(path1, NULL, 1, &inode);
	if (err) {
		return err;
	}
	if (!S_ISREG(inode->i_mode)) {
		vfs_iput(inode);
		return -EPERM;
	}
	err = vfs_dirnamei(path2, NULL, &basename, &dir);
	if (err) {
		vfs_iput(inode);
		return err;
	}
	if (*basename == '\0') {
		vfs_iput(dir);
		vfs_iput(inode);
		return -EPERM;
	}
	if (dir->i_dev != inode->i_dev) {
		vfs_iput(dir);
		vfs_iput(inode);
		return -EXDEV;
	}
	if (!check_permission(dir, MAY_WRITE)) {
		vfs_iput(dir);
		vfs_iput(inode);
		return -EACCES;
	}
    if (!dir->i_ops || !dir->i_ops->link) {
		vfs_iput(dir);
		vfs_iput(inode);
		return -ENOENT;
	}
    return dir->i_ops->link(dir, basename, inode);
}

i32 syscall_rename(i8 *oldname, i8 *newname) {
	struct vfs_inode *old_dir, *new_dir;
	const i8 *old_base, *new_base;
	i32 error;

	error = vfs_dirnamei(oldname, NULL, &old_base, &old_dir);
	if (error) {
		return error;
	}
	if (!check_permission(old_dir, MAY_WRITE | MAY_EXEC)) {
		vfs_iput(old_dir);
		return -EACCES;
	}
	if (*old_base == '\0' || (old_base[0] == '.' && strlen(old_base) == 1) ||
			(old_base[1] == '.' && strlen(old_base) == 2)) {
		vfs_iput(old_dir);
		return -EPERM;
	}
	error = vfs_dirnamei(newname, NULL, &new_base, &new_dir);
	if (error) {
		return error;
	}
	if (!check_permission(new_dir, MAY_WRITE | MAY_EXEC)) {
		vfs_iput(new_dir);
		vfs_iput(old_dir);
		return -EACCES;
	}
	if (*new_base == '\0' || (new_base[0] == '.' && strlen(new_base) == 1) ||
			(new_base[1] == '.' && strlen(new_base) == 2)) {
		vfs_iput(new_dir);
		vfs_iput(old_dir);
		return -EPERM;
	}
	if (old_dir->i_dev != new_dir->i_dev) {
		vfs_iput(new_dir);
		vfs_iput(old_dir);
		return -EXDEV;
	}
	if (!old_dir->i_ops || !old_dir->i_ops->rename) {
		vfs_iput(old_dir);
    	vfs_iput(new_dir);
		return -EPERM;
	}
    return old_dir->i_ops->rename(old_dir, old_base, new_dir, new_base);
}

struct dirent *syscall_readdir(DIR *dirp) {
	struct file *file;
	struct vfs_inode *inode;
	i32 i;

	if (dirp->fd >= NR_OPEN ||
			!(file = current_process->fds[dirp->fd]) ||
			!(inode = file->f_inode)) {
		return NULL;
	}
    if (!file->f_ops || !file->f_ops->readdir) {
		return NULL;
    }
	i = file->f_ops->readdir(inode, file, &dirp->dent);
	if (i) {
		return &dirp->dent;
	} else {
		return NULL;
	}
}

i32 syscall_test(i32 n) {
	debug("START syscall_test()\r\n");
	kprintf("Got n - %d\r\n", n);
	debug("END syscall_test()\r\n");
	return 0;
}

i32 syscall_access(i8 *path, i32 amode) {
	struct vfs_inode *inode;
	i32 err, res, i_mode;

	amode &= 0007;
	if ((err = vfs_namei(path, NULL, 1, &inode))) {
		return err;
	}
	i_mode = res = inode->i_mode & 0777;
	if (current_process->uid == inode->i_uid) {
		res >>= 6;
	} else if (current_process->gid == inode->i_gid) {
		res >>= 3;
	}
	if ((res & 0007 & amode) == i_mode) {
		vfs_iput(inode);
		return 0;
	}
	if (!current_process->uid &&
			(!(amode & 1) || (i_mode & 0111))) {
		vfs_iput(inode);
		return 0;
	}
	vfs_iput(inode);
	return -EACCES;
}

i32 syscall_chmod(i8 *path, i32 mode) {
	i32 err;
	struct vfs_inode *inode;

	if ((err = vfs_namei(path, NULL, 1, &inode))) {
		return err;
	}
	if ((current_process->euid != inode->i_uid) && current_process->uid != 0) {
		vfs_iput(inode);
		return -EACCES;
	}
	inode->i_mode = (mode & 07777) | (inode->i_mode & ~07777);
	inode->i_ctime = get_current_time();
	inode->i_dirt = 1;
	vfs_iput(inode);
	return 0;
}

i32 syscall_chown(i8 *path, i32 owner, i32 group) {
	i32 err;
	struct vfs_inode *inode;

	if ((err = vfs_namei(path, NULL, 1, &inode))) {
		return err;
	}
	if (current_process->euid != inode->i_uid && current_process->uid != 0) {
		vfs_iput(inode);
		return -EACCES;
	}
	inode->i_uid = owner;
	inode->i_gid = group;
	inode->i_dirt = 1;
	vfs_iput(inode);
	return 0;
}

i32 syscall_utime(i8 *path, struct utimbuf *times) {
	struct vfs_inode *inode;
	i32 err;
	u32 actime, modtime;

	if ((err = vfs_namei(path, NULL, 1, &inode))) {
		return err;
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
	vfs_iput(inode);
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

i32 syscall_rmdir(i8 *path) {
	i32 retval;
	const i8 *basename;
	struct vfs_inode *dir;

	if ((retval = vfs_dirnamei(path, NULL, &basename, &dir))) {
		return retval;
	}
	if (*basename == '\0') {
		vfs_iput(dir);
		return -ENOENT;
	}
	if (!check_permission(dir, MAY_WRITE)) {
		vfs_iput(dir);
		return -EACCES;
	}
    if (!dir->i_ops || !dir->i_ops->rmdir) {
		vfs_iput(dir);
		return -EPERM;
	}
    return dir->i_ops->rmdir(dir, basename);
}

i32 syscall_mkdir(i8 *path, i32 mode) {
	i32 err;
	const i8 *basename;
	struct vfs_inode *dir;

	if ((err = vfs_dirnamei(path, NULL, &basename, &dir))) {
		return err;
	}
	if (*basename == '\0') {
		vfs_iput(dir);
		return -ENOENT;
	}
    if (!dir->i_ops || !dir->i_ops->mkdir) {
		vfs_iput(dir);
		return -EPERM;
	}
    return dir->i_ops->mkdir(dir, basename, mode);
}

i32 syscall_pipe(i32 fidles[2]) {
	struct vfs_inode *inode;
	struct file *f[2];
	i32 fd[2];
	i32 i, j;

	j = 0;
	for (i = 0; j < 2 && i < NR_FILE; ++i) {
		if (!file_table[i].f_count) {
			f[j] = file_table + i;
			++f[j]->f_count;
			++j;
		}
	}
	if (j == 1) {
		f[0]->f_count = 0;
	}
	if (j < 2) {
		return -1;
	}
	j = 0;
	for (i = 3; j < 2 && i < NR_OPEN; ++i) {
		if (!current_process->fds[i]) {
			fd[j] = i;
			current_process->fds[i] = f[j];
			++j;
		}
	}
	if (j == 1) {
		current_process->fds[fd[0]] = NULL;
	}
	if (j < 2) {
		f[0]->f_count=f[1]->f_count = 0;
		return -1;
	}

	if (!(inode = get_pipe_inode())) {
		current_process->fds[fd[0]] = current_process->fds[fd[1]] = NULL;
		f[0]->f_count=f[1]->f_count = 0;
		return -1;
	}
	f[0]->f_inode = f[1]->f_inode = inode;
	f[0]->f_pos = f[1]->f_pos = 0;


	f[0]->f_mode = 1; /* read */
	f[0]->f_ops = &pipe_read_ops;
	f[1]->f_mode = 2; /* write */
	f[1]->f_ops = &pipe_write_ops;


	fidles[0] = fd[0];
	fidles[1] = fd[1];
	return 0;
}

i32 syscall_tcsetpgrp(i32 fildes, i32 pgrp_id) {
	struct file *f;
	if (fildes > NR_OPEN) {
		return -EBADF;
	}
	f = current_process->fds[fildes];
	if (!f) {
		return -EBADF;
	}
	if (current_process->tty < 0) {
		return -ENOTTY;
	}
	tty_table[current_process->tty].pgrp = pgrp_id;
	return 0;
}


i32 syscall_tcgetpgrp(i32 fildes) {
	struct file *f;
	if (fildes > NR_OPEN) {
		return -EBADF;
	}
	f = current_process->fds[fildes];
	if (!f) {
		return -EBADF;
	}
	if (current_process->tty < 0) {
		return -ENOTTY;
	}
	return tty_table[current_process->tty].pgrp;
}

i32 syscall_symlink(i8 *path1, i8 *path2) {
	const i8 *basename;
	i32 err;
	struct vfs_inode *dir;

	err = vfs_dirnamei(path2, NULL, &basename, &dir);
	if (err) {
		return err;
	}
	if (*basename == '\0') {
		vfs_iput(dir);
		return -ENOENT;
	}
	if (!check_permission(dir, MAY_WRITE)) {
		vfs_iput(dir);
		return -EPERM;
	}
	if (!dir->i_ops || !dir->i_ops->symlink) {
		vfs_iput(dir);
		return -ENOENT;
	}
    return dir->i_ops->symlink(dir, basename, path1);
}

i32 syscall_readlink(i8 *pathname, i8 *buf, i32 bufsiz) {
	i32 err;
	struct vfs_inode *inode;

	if (bufsiz <= 0) {
		return -EINVAL;
	}
	if ((err = vfs_namei(pathname, NULL, 0, &inode))) {
		return err;
	}
	if (!inode->i_ops || !inode->i_ops->readlink) {
		vfs_iput(inode);
		return -EINVAL;
	}
    return inode->i_ops->readlink(inode, buf, bufsiz);
}

i32 syscall_lstat(i8 *path, struct stat *statbuf) {
	i32 err;
	struct vfs_inode *inode;

	if ((err = vfs_namei(path, NULL, 0, &inode))) {
		return err;
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
	statbuf->st_blksize = inode->i_sb->s_block_size;
	statbuf->st_blocks = inode->i_blocks;
	vfs_iput(inode);
	return 0;
}

i32 syscall_truncate(const i8 *path, u32 length) {
	i32 err;
	struct vfs_inode *inode;

	if ((err = vfs_namei(path, NULL, 1, &inode))) {
		return err;
	}
	if (!inode->i_ops || !inode->i_ops->truncate) {
		vfs_iput(inode);
		return -EINVAL;
	}
        return inode->i_ops->truncate(inode, length);
}
