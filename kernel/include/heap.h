#ifndef HEAP_H
#define HEAP_H

#include "types.h"
#include <common.h>

#define HEAP_START          0xC0400000
#define HEAP_END            0xC0500000

struct heap_block {
    u32 size;
    struct heap_block *next;
    bool free;
};

void heap_init(void);
void *malloc(u32 size);
void free(void *ptr);
void *realloc(void *ptr, u32 size);

#endif
