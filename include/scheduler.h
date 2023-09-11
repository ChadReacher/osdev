#ifndef SCHEDULER_H
#define SCHEDULER_H

#include <types.h>
#include <process.h>
#include <isr.h>

void scheduler_init();
void schedule(registers_state *regs);
void task_switch(process_t *next_proc);

void add_process_to_list(process_t *new_proc);
void remove_process_from_list(process_t *proc);

#endif
