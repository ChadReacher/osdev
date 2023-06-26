#ifndef TIMER_H
#define TIMER_H

#include "types.h"

#define COUNTER_0_REGISTER 0x40
#define CONTROL_WORD_REGISTER 0x43
#define INPUT_FREQUENCY 1193180 // Hz

void init_timer(u32 freq);

#endif
