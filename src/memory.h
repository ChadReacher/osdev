#ifndef MEMORY_H
#define MEMORY_H

#include "types.h"

void *memset(void *ptr, u32 value, u32 num);
void *memcpy(void *dst, const void* src, u32 num); 

#endif
