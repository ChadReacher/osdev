#ifndef PANIC_H
#define PANIC_H

#include "types.h"

#define __PANIC(format, ...) kernel_panic("\nPANIC in %s:%d:%s(): " \
							 format "\n" "%s", \
							 __FILE__, __LINE__, __func__, __VA_ARGS__);

#define PANIC(...) __PANIC(__VA_ARGS__, "")

void kernel_panic(i8 *fmt, ...);

#endif
