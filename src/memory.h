#ifndef MEMORY_H
#define MEMORY_H

#include "util.h"

void *memset(void *ptr, u32 value, size_t num);
void *memcpy(void *dst, const void* src, size_t num); 

#endif
