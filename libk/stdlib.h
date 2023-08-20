#ifndef STDLIB_H
#define STDLIB_H

#include "types.h"

i8 *itoa(i32 value, i8 *str, i32 base);
i8 *utoa(u32 value, i8 *str, u32 base);
u32 atoi(const i8 *str);

#endif
