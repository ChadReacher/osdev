#ifndef TIMER_H
#define TIMER_H

#include "types.h"

#define COUNTER_0_REGISTER 0x40
#define CONTROL_WORD_REGISTER 0x43
#define INPUT_FREQUENCY_IN_HZ 1193180 // Hz

#define TIMER_FREQ 100

void timer_init(u32 freq);
u32 get_current_time();

#endif
