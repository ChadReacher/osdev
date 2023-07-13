#ifndef PMM_H
#define PMM_H

#include "types.h"
#include "bios_memory_map.h"

extern u32 *memory_map;
extern u32 total_blocks;
extern u32 used_blocks;

#define BLOCK_SIZE		4096
#define BLOCKS_PER_BYTE 8

#define INDEX_FROM_BIT(b) (b / 32)
#define OFFSET_FROM_BIT(b) (b % 32)

void _pmm_init(u32 start_address, u32 size);
void pmm_init();
void set_block(u32 bit);
void clear_block(u32 bit);
u8 test_block(u32 bit);
i32 find_first_free_blocks(u32 num_blocks);
void mark_memory_as_free(u32 base_address, u32 size);
void mark_memory_as_used(u32 base_address, u32 size);
void *allocate_blocks(u32 num_blocks);
void free_blocks(void *address, u32 num_blocks);

#endif
