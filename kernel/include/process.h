#ifndef PROCESS_H
#define PROCESS_H

#include <types.h>
#include <paging.h>
#include <isr.h>
#include <signal.h>
#include "ext2.h" 

#define ALIGN_UP(val, a) (((val) + ((a) - 1)) & ~((a) - 1))
#define ALIGN_DOWN(val, a) ((val) & ~((a) - 1))

typedef enum {
	RUNNING,
	STOPPED,
	INTERRUPTIBLE,
	ZOMBIE
} state_t;

typedef struct {
	u32 edi;
	u32 esi;
	u32 ebx;
	u32 ebp;
	u32 eip;
} context_t;

#define NR_OPEN 20
#define NR_FILE 32
typedef struct _process {
	i32 pid;
	i32 timeslice;
	state_t state;
	i32 exit_code;
	struct _process *parent;
	page_directory_t *directory;
	registers_state *regs;
	context_t *context;
	void *kernel_stack_bottom;
	void *kernel_stack_top;
	u32 close_on_exec;
	struct file *fds[NR_OPEN];
	struct ext2_inode *root;
	struct ext2_inode *pwd;
	char *str_pwd;
	u32 brk;
	u32 umask;
	sigset_t sigpending;
	sigset_t sigmask;
	sigaction_t signals[NSIG];
	u32 *sigreturn;
	sigset_t old_sigmask;
	registers_state signal_old_regs;
	u32 sleep;
	i32 alarm;
	u16 uid, euid;
	u8 gid, egid;
	i32 pgrp, session, leader;
	i32 utime, stime, cutime, cstime;
} process_t;

void user_init();
void enter_usermode();
i32 get_new_fd();
struct file *get_empty_file();

#endif
