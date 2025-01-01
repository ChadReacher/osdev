#ifndef SCHEDULER_H
#define SCHEDULER_H

#include <types.h>
#include <process.h>

void scheduler_init();
void schedule();
struct proc *get_proc_by_id(i32 pid);
int get_free_proc();
i32 handle_signal(struct registers_state *regs);
i32 send_signal(struct proc *proc, i32 sig);

#endif
