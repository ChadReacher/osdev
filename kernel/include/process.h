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
// * RUNNING - a process is either running on CPU or ready to run
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
    // Process identifier, 0 - IDLE, 1 - INIT, 2,3,.. - other processes
    i32 pid;
    // Remaining time slice (timer ticks) a process will spend on a CPU
    i32 timeslice;
    // Current process state
    enum state state;
    // Exit code so parent could reap it from a zombie process
    i32 exit_code;
    // Parent process
    struct proc *parent;
    // Page directory for paging (virtual memory)
    physical_address page_directory;
    // Saved CPU registers and some extra registers from an interrupt
    struct registers_state *regs;
    // Registers context used for the "context switch"
    struct context *context;
    // Kernel stack pointers: bottom pointer is used to release the acquired memory
    void *kernel_stack_bottom;
    // Top stack pointer is used to update the TSS before context switch
    void *kernel_stack_top;
    // Open file descriptors
    struct file *fds[NR_OPEN];
    // Root directory
    struct vfs_inode *root;
    // Current working directory
    struct vfs_inode *pwd;
    // String representation of `pwd`
    // TODO: remove and update the syscall_getcwd()
    char *str_pwd;
    // Break of the process memory: first unallocated byte
    // TODO: should it be page aligned?
    u32 brk;
    // File mode creation mask
    u32 umask;
    // List of pending signals
    sigset_t sigpending;
    // List of blocked signals
    sigset_t sigmask;
    // List of actions for signals (either default, ignore or custom handler)
    sigaction_t signals[NSIG];
    // Address where to return from signal handler
    u32 *sigreturn;
    // Temporary signal mask when executing signal handler
    sigset_t old_sigmask;
    // Saved registers before executing signal handler
    struct registers_state signal_old_regs;
    // Number of ticks a process should be blocked for
    u32 sleep;
    // Time (number of ticks) when kernel should send SIGALARM signal to the process
    i32 alarm;
    // Real user id, effective user id
    u16 uid, euid;
    // Real group id, effective group id
    u8 gid, egid;
    // Process group id, session id
    // TODO: rename -> pgid, sid
    // TODO: remove `leader` field as leader means session leader
    // and session leader is process where `session id` == `process id` i.e. `session` == `pid`
    i32 pgrp, session, leader;
    // Process groups to which a process belongs to
    i32 groups[NR_GROUPS];
    // Time spent in user space, measured in ticks
    i32 utime;
    // Time spent in kernel space, measured in ticks
    i32 stime;
    // Children process's time spent in user space, measure in ticks
    i32 cutime;
    // Children process's time spent in kernel space, measure in ticks
    i32 cstime;
    // Controlling terminal number
    // TODO: should it have `dev_t` type? and assign `inode->i_dev`
    // look at pm_attach_tty
    // Probably used to:
    // a) check the background read `handle_background_read`
    // b) attach tty
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
