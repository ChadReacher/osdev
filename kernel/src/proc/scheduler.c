#include "paging.h"
#include <scheduler.h>
#include <tss.h>
#include <syscall.h>
#include <stdlib.h>
#include <string.h>
#include <heap.h>
#include <process.h>
#include <panic.h>

extern u32 next_pid;
extern void irq_ret(void);
extern void switch_to(struct context **old_context, struct context *new_context);

struct proc *procs[NR_PROCS];
struct proc *current_process = NULL;

static void task_switch(struct proc *next_proc);

static void cpu_idle(void) {
    while (1) {
        __asm__ volatile ("hlt");
    }
}

struct proc *get_proc_by_id(i32 pid) {
    if (pid < 0 || pid > NR_PROCS) {
        return NULL;
    }
    return procs[pid];
}

int get_free_proc(void) {
    for (u32 i = 0; i < NR_PROCS; ++i) {
        if (procs[i] == NULL) {
            return i;
        }
    }
    return -1;
}

static void UNUSED dump_procs(void) {
    int i;
    debug("Queue of all processes:\r\n");
    for (i = 0; i < NR_PROCS; ++i) {
        char *state;
        struct proc *p = procs[i];
        if (!p) {
            continue;
        }
        switch (p->state) {
            case RUNNING:
                state = "RUNNING";
                break;
            case ZOMBIE:
                state = "ZOMBIE";
                break;
            case INTERRUPTIBLE:
                state = "INTERRUPTIBLE";
                break;
            case STOPPED:
                state = "STOPPED";
                break;
            default:
                state = "UNKNOWN";
                break;
        }
        debug("Process(%p) with PID %d, state: %s\r\n",
                p, p->pid, state);
    }
}

void schedule(void) {
    struct proc *next_proc = NULL;
    struct proc *p = NULL;

    while (1) {
        i32 count = -1;
        next_proc = procs[0];
        for (u32 i = 1; i < NR_PROCS; ++i) {
            p = procs[i];
            if (!p) {
                continue;
            }
            if (p->state == RUNNING && p->timeslice > count) {
                next_proc = p;
                count = p->timeslice;
            }
        }
        if (count) {
            break;
        }
        for (u32 i = 1; i < NR_PROCS; ++i) {
            p = procs[i];
            if (p) {
                p->timeslice = DEFAULT_TIMESLICE;
            }
        }
    }

    if (next_proc != current_process) {
        task_switch(next_proc);
    }
}

static void task_switch(struct proc *next_proc) {
    struct proc *prev_proc = current_process;
    current_process = next_proc;

    tss_set_stack((u32)current_process->kernel_stack_top);
    __asm__ volatile ("movl %%eax, %%cr3"
            : 
            : "a"(current_process->page_directory));
    switch_to(&prev_proc->context, current_process->context);
}

static void create_idle_process(void) {
    struct proc *idle_process = procs[IDLE_PID] = malloc(sizeof(struct proc));
    assert(idle_process != NULL);
    memset(idle_process, 0, sizeof(struct proc));

    idle_process->pid = next_pid++;
    idle_process->timeslice = DEFAULT_TIMESLICE;
    idle_process->state = RUNNING;
    idle_process->page_directory = virtual_to_physical(CURR_PAGE_DIR);

    idle_process->kernel_stack_bottom = malloc(KSTACK_SZ);
    assert(idle_process->kernel_stack_bottom != NULL);
    memset(idle_process->kernel_stack_bottom, 0, KSTACK_SZ);

    u8 *kstack_top = (u8 *)ALIGN_DOWN(
        (u32)idle_process->kernel_stack_bottom + KSTACK_SZ - 1, sizeof(u32));

    kstack_top -= sizeof(struct registers_state) + sizeof(u32);
    idle_process->regs = (struct registers_state *)kstack_top;
    idle_process->regs->eflags = 0x202;
    idle_process->regs->cs = KERNEL_CS;
    idle_process->regs->ds = KERNEL_DS;
    idle_process->regs->es = KERNEL_CS;
    idle_process->regs->fs = KERNEL_CS;
    idle_process->regs->gs = KERNEL_CS;
    idle_process->regs->eip = (u32)cpu_idle;

    kstack_top -= sizeof(struct context);
    idle_process->context = (struct context *)kstack_top;
    idle_process->context->eip = (u32)irq_ret;

    idle_process->kernel_stack_top = kstack_top;
}

static void create_init_process(void) {
    struct proc *init_process = current_process = procs[1] = malloc(sizeof(struct proc));
    assert(init_process != NULL);
    memset(init_process, 0, sizeof(struct proc));

    init_process->pid = next_pid++;
    init_process->timeslice = DEFAULT_TIMESLICE;
    init_process->state = RUNNING;
    for (u32 i = 0; i < NR_GROUPS; ++i) {
        init_process->groups[i] = -1;
    }
    init_process->tty = -1;

    init_process->kernel_stack_bottom = malloc(KSTACK_SZ);
    assert(init_process->kernel_stack_bottom != NULL);
    memset(init_process->kernel_stack_bottom, 0, KSTACK_SZ);

    u8 *kstack_top = (u8 *)ALIGN_DOWN(
        (u32)init_process->kernel_stack_bottom + KSTACK_SZ - 1, sizeof(u32));

    kstack_top -= sizeof(struct registers_state) + sizeof(u32);
    init_process->regs = (struct registers_state *)kstack_top;

    kstack_top -= sizeof(struct context);
    init_process->context = (struct context *)kstack_top;

    init_process->kernel_stack_top = kstack_top;
}

void scheduler_init(void) {
    create_idle_process();
    create_init_process();

    debug("Scheduler has been successfully initialized\r\n");
}
