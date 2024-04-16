#ifndef SCHEDULER_H
#define SCHEDULER_H

#include <types.h>
#include <process.h>

void scheduler_init();
void schedule();
process_t *get_proc_by_id(i32 pid);
void wake_up(process_t **p);
void goto_sleep(process_t **p);
int get_free_proc();
i32 handle_signal(registers_state *regs);
i32 send_signal(process_t *proc, i32 sig);

#endif
