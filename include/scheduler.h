#ifndef SCHEDULER_H
#define SCHEDULER_H

#include <types.h>
#include <process.h>
#include <isr.h>

void run_init_process(i8 *file);
void scheduler_init();
process_t *get_current_process();
void add_process_to_list(process_t *new_proc);
void schedule(registers_state *regs);
void process_kill(process_t *proc);

#endif
