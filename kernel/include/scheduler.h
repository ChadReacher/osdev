#ifndef SCHEDULER_H
#define SCHEDULER_H

#include <types.h>
#include <process.h>

void scheduler_init();
void schedule();
i32 handle_signal();
u32 send_signal(process_t *proc, i32 sig);
process_t *get_proc_by_id(u32 pid);

#endif
