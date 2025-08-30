#ifndef PANIC_H
#define PANIC_H

#include "types.h"

#define UNUSED __attribute__((__unused__))

void debug(i8 *fmt, ...);
void panic(i8 *fmt, ...);
void stack_trace(void);

#define assert(expr) do { \
    if (!(expr)) { \
        panic("Assertion failed: [" #expr "] at (%s:%s:%d)\r\n", __FILE__, __func__, __LINE__); \
    } \
} while (0)

#endif
