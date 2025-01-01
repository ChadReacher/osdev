#ifndef PROCESS_H
#define PROCESS_H

#include <types.h>
#include <paging.h>
#include <isr.h>
#include <signal.h>
#include <vfs.h> 

#define NR_OPEN 20
#define NR_FILE 32
#define NR_GROUPS 32
#define NR_PROCS 32

#define ALIGN_UP(val, a) (((val) + ((a) - 1)) & ~((a) - 1))
#define ALIGN_DOWN(val, a) ((val) & ~((a) - 1))

enum state {
	RUNNING,
	STOPPED,
	INTERRUPTIBLE,
	ZOMBIE
};

struct context {
	u32 edi;
	u32 esi;
	u32 ebx;
	u32 ebp;
	u32 eip;
};

struct proc {
	i32 pid;
	i32 timeslice;
	enum state state;
	i32 exit_code;
	struct proc *parent;
	page_directory_t *directory;
	struct registers_state *regs;
	struct context *context;
	void *kernel_stack_bottom;
	void *kernel_stack_top;
	u32 close_on_exec;
	struct file *fds[NR_OPEN];
	struct vfs_inode *root;
	struct vfs_inode *pwd;
	char *str_pwd;
	u32 symlink_count;
	u32 brk;
	u32 umask;
	sigset_t sigpending;
	sigset_t sigmask;
	sigaction_t signals[NSIG];
	u32 *sigreturn;
	sigset_t old_sigmask;
	struct registers_state signal_old_regs;
	u32 sleep;
	i32 alarm;
	u16 uid, euid;
	u8 gid, egid;
	i32 pgrp, session, leader;
	i32 groups[NR_GROUPS];
	i32 utime, stime, cutime, cstime;
	i32 tty;
};

void user_init();
void enter_usermode();
i32 get_new_fd();
struct file *get_empty_file();

void process_wakeup(struct proc **p);
void process_sleep(struct proc **p);

extern struct proc *current_process;

#endif
