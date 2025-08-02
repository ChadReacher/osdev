#ifndef PANIC_H
#define PANIC_H

#include "types.h"

void debug(i8 *fmt, ...);
void panic(i8 *fmt, ...);

#define assert(expr) do { \
    if (!(expr)) { \
        panic("Assertion failed: [" #expr "] at (%s:%s:%d)\r\n", __FILE__, __func__, __LINE__); \
    } \
} while (0)

#define UNUSED __attribute__((__unused__))

#endif
