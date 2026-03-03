#ifndef LOCK_H
#define LOCK_H

#include <types.h>

extern u32 irq_counter;

void gl_lock(void);
void gl_unlock(void);

#endif // LOCK_H