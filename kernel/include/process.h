#ifndef PROCESS_H
#define PROCESS_H

#include <types.h>
#include <paging.h>
#include <isr.h>
#include <signal.h>
#include <vfs.h> 

#define IDLE_PID 0
#define INIT_PID 1

#define INIT_PROGRAM "/bin/init"

// A slice of time allocated for a process to continously run on CPU
// (measured in timer ticks)
#define DEFAULT_TIMESLICE 20

// Kernel stack size in bytes
#define KSTACK_SZ (2 * PAGE_SIZE)

// A process-wide maximum number of opened file descriptors
#define NR_OPEN 20
// A process-wide maximum number of groups
#define NR_GROUPS 32

// A system-wide maximum number of available procceses
#define NR_PROCS 32
// A system-wide maximum number of opened files
#define NR_FILE 32

// The possible states a process can be in:
// * RUNNING - a process either is running on CPU or ready to run
// * STOPPED - a process is stopped due to respective signals
// * INTERRUPTIBLE - a process is sleeping until an event occurs
// * ZOMBIE - a process which parent didn't read the return code
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
	physical_address page_directory;
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

void user_init(void);
void enter_usermode(void);

// Allocates a new file descriptor
i32 process_fd_new(void);
// Allocates a new file
struct file *process_file_new(void);
// Wakes up the process
void process_wakeup(struct proc *p);
// Puts the process to sleep
void process_sleep(void);

extern struct proc *current_process;

#endif
