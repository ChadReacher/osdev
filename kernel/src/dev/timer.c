#include <timer.h>
#include <isr.h>
#include <port.h>
#include <panic.h>
#include <process.h>
#include <scheduler.h>

// It is initialized by CMOS RTC
u32 startup_time;

// It contains the ticks the timer made
u32 ticks = 0;

static void timer_handler(struct registers_state *regs) {
    assert(current_process != NULL);

    ++ticks;

    if (regs->cs & 0x3) {
        ++current_process->utime;
    } else {
        ++current_process->stime;
    }

    if ((--current_process->timeslice) > 0) {
        return;
    }
    current_process->timeslice = 0;
    schedule();
}

u32 get_current_time(void) {
    return startup_time + ticks / TIMER_FREQ_HZ;
}

void timer_init(void) {
    u8 command_word, low_byte, high_byte;
    u16 divisor;

    divisor = INPUT_FREQUENCY_HZ / TIMER_FREQ_HZ;
    low_byte = (u8)(divisor & 0xFF);
    high_byte = (u8)((divisor >> 8) & 0xFF);

    command_word = 0x36;
    /* 0011 0110
     * Set binary counting, set mode 3 (Square Wave Generator),
     * read LSB first then MSB, select Counter 0
     */
    port_outb(CONTROL_WORD_REGISTER, command_word);

    port_outb(COUNTER_0_REGISTER, low_byte);
    port_outb(COUNTER_0_REGISTER, high_byte);

    register_interrupt_handler(IRQ0, timer_handler);

    debug("Programmable Interrupt Timer has been initialized\r\n");
}
