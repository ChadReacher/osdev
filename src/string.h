#ifndef STRING_H
#define STRING_H

#include "types.h"

size_t strlen(const i8 *str);
i8 *strrev(i8 *str);
u8 strncmp(const i8 *left, const i8 *right, i32 len);
i8 *strcpy(i8 *dest, const i8 *src);

#endif
