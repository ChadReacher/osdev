#ifndef TIMER_H
#define TIMER_H

#include "types.h"

#define COUNTER_0_REGISTER 0x40
#define CONTROL_WORD_REGISTER 0x43
#define INPUT_FREQUENCY_HZ 1193180 /* Hz */

// Timer frequency in Hz
// It will fire an interrupt 100 times in one second
#define TIMER_FREQ_HZ 100

void timer_init(void);
u32 get_current_time(void);

#endif
