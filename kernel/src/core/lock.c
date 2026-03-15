#include <panic.h>
#include <lock.h>
#include <common.h>

// Global kernel lock

// Counter to correctly handle nested locks
u32 irq_counter = 0;

void gl_lock(void) {
    disable_interrupts();
    ++irq_counter;
}

void gl_unlock(void) {
    assert(irq_counter != 0);

    --irq_counter;
    if (irq_counter == 0) {
        enable_interrupts();
    }
}
