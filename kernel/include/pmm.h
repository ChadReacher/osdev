#ifndef PMM_H
#define PMM_H

#include "types.h"

#define PMM_BLOCK_SIZE	4096

void pmm_init(void);
void *allocate_blocks(u32 num_blocks);
void free_blocks(void *address, u32 num_blocks);

#endif
