#include <process.h>
#include <heap.h>
#include <tss.h>
#include <panic.h>
#include <scheduler.h>

extern struct proc *procs[NR_PROCS];
extern void enter_usermode_asm(u32 useresp);

i32 syscall_exec(i8 *pathname, i8 **u_argv, i8 **u_envp);

u32 next_pid = 0;
struct file file_table[NR_FILE];


void process_sleep(void) {
    current_process->state = INTERRUPTIBLE;
    schedule();
}

void process_wakeup(struct proc *p) {
    if (!p) {
        return;
    }
    p->state = RUNNING;
}

i32 process_fd_new(void) {
    i32 fd = 0;
    for (fd = 0; fd < NR_OPEN; ++fd) {
        if (!current_process->fds[fd]) {
            break;
        }
    }
    return fd;
}

struct file *process_file_new(void) {
    for (i32 i = 0; i < NR_FILE; ++i) {
        if (!file_table[i].f_count) {
            return &file_table[i];
        }
    }
    return NULL;
}

void enter_usermode(void) {
    current_process = procs[INIT_PID];

    tss_set_stack((u32)current_process->kernel_stack_top);
    __asm__ volatile ("movl %%eax, %%cr3" : : "a"(current_process->page_directory));

    debug("Entering user space with INIT process\r\n");
    enter_usermode_asm(current_process->regs->useresp);
}

void user_init(void) {
    i8 *argv[] = { INIT_PROGRAM, NULL };
    i8 *envp[] = { "PATH=/bin", NULL };
    struct proc *init_process = procs[INIT_PID];

    init_process->sid = init_process->pgid = init_process->pid;

    physical_address curr_page_dir = virtual_to_physical(CURR_PAGE_DIR);
    init_process->page_directory = (physical_address) paging_copy_page_dir(0);

    __asm__ volatile ("movl %%eax, %%cr3" : : "a"(init_process->page_directory));

    i32 err = syscall_exec(INIT_PROGRAM, argv, envp);
    assert(err == 0);

    __asm__ volatile ("movl %%eax, %%cr3" : : "a"(curr_page_dir));
}
