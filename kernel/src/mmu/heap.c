#include <heap.h>
#include <pmm.h>
#include <paging.h>
#include <panic.h>
#include <string.h>
#include <common.h>

static void *heap_curr;

static struct heap_block *block_list;

static struct heap_block *best_fit(u32 size);
static void split_block(struct heap_block *block, u32 size);

void heap_init(void) {
    heap_curr = (void *)HEAP_START;

    u32 heap_size = HEAP_END - HEAP_START;
    u32 heap_blocks = heap_size / PMM_BLOCK_SIZE;
    void *heap_ptr = allocate_blocks(heap_blocks);
    if (heap_ptr == NULL) {
        panic("Failed to allocate %d physical blocks: not enough space :(", heap_blocks);
    }
    virtual_address heap_vaddr = (u32)HEAP_START;
    virtual_address heap_paddr = (u32)heap_ptr;

    for (u32 i = 0; i < heap_blocks; ++i) {
        map_page(heap_paddr, heap_vaddr, PAGING_FLAG_PRESENT | PAGING_FLAG_WRITEABLE);
        heap_vaddr += PAGE_SIZE;
        heap_paddr += PMM_BLOCK_SIZE;
    }

    debug("Heap size - %d KiB\r\n", (heap_size / KIB));
    debug("Heap in virtual  space: %#010x - %#010x\r\n", HEAP_START, HEAP_END); 
    debug("Heap in physical space: %#010x - %#010x\r\n", heap_ptr, (u8*)heap_ptr + heap_size); 

    debug("Kernel heap has been initialized\r\n");
}

void *sbrk(u32 increment) {
    void *old_boundary = heap_curr;
    u32 new_boundary = (u32)heap_curr + increment;

    if (new_boundary <= HEAP_END) {
        heap_curr = (void *)new_boundary;
        return old_boundary;
    } else {
        panic("Ooops, we have reached the HEAP END\r\n");
    }
    return NULL;
}

static struct heap_block *best_fit(u32 size) {
    struct heap_block *current = block_list;
    struct heap_block *best_current = NULL;
    while (current) {
        if (current->free && current->size >= size) {
            if (best_current == NULL || current->size < best_current->size) {
                best_current = current;
            }
        }
        current = current->next;
    }
    return best_current;
}

static void split_block(struct heap_block *block, u32 size) {
    if (block->size > sizeof(struct heap_block) + size) {
        struct heap_block *splited_block = (struct heap_block *)((u8*)block + sizeof(struct heap_block) + size);
        splited_block->free = 1;
        splited_block->size = block->size - size - sizeof(struct heap_block);
        splited_block->next = block->next;
        block->size = size;
        block->next = splited_block;
    }
}

/* 
 * Allocate 'size' bytes
 * It allocates a header with needed number of bytes
 * So first it goes a header, then after the header
 * real allocated space goes.
 */
void *malloc(u32 size) {
    struct heap_block *block = NULL;

    assert(size > 0);

    u32 real_size = sizeof(struct heap_block) + size;
    real_size = ALIGN(real_size);

    if (block_list) {
        struct heap_block* best_block = best_fit(size);
        if (best_block) {
            assert(best_block->free == 1);
            best_block->free = 0;
            split_block(best_block, size);
            block = best_block;
        } else {
            block = sbrk(real_size);
            if (!block) {
                return NULL;
            }
            block->size = size;
            block->next = NULL;
            block->free = 0;
            struct heap_block *last = block_list;
            while (last->next) {
                last = last->next;
            }
            last->next = block;
        }
    } else {
        block = sbrk(real_size);
        if (!block) {
            return NULL;
        }
        block->size = size;
        block->next = NULL;
        block->free = 0;
        block_list = block;
    }

    if (block) {
        return block + 1; 
    }
    return NULL;
}

void free(void *ptr) {
    if (!ptr) {
        return;
    }

    struct heap_block *block = (struct heap_block *)((u8 *)ptr - sizeof(struct heap_block));
    assert(block->free == 0);
    block->free = 1;

    struct heap_block *prev_block = NULL;
    struct heap_block *curr_block = block_list;
    struct heap_block *next_block = block_list->next;
    while (curr_block && next_block) {
        if (curr_block == block) {
            break;
        }
        prev_block = curr_block;
        curr_block = next_block;
        next_block = curr_block->next;
    }

    // Merge the current block with next block
    if (next_block && next_block->free) {
        curr_block->size = curr_block->size + sizeof(struct heap_block) + next_block->size;
        curr_block->next = next_block->next;
    }
    // Merge the current block with previous block
    if (prev_block && prev_block->free) {
        prev_block->size = prev_block->size + sizeof(struct heap_block) + curr_block->size;
        prev_block->next = curr_block->next;
    }
}

void *realloc(void *ptr, u32 size) {
    if (!ptr) {
        return malloc(size);
    } else if (ptr && size == 0) {
        free(ptr);
        return NULL;
    }

    struct heap_block *old_block = (struct heap_block *)((u8 *)ptr - sizeof(struct heap_block));
    u32 old_size = old_block->size;
    u32 real_size = sizeof(struct heap_block) + size;
    
    struct heap_block *block = best_fit(size);
    if (block) {
        assert(block->free == 1);
        block->free = 0;
        split_block(block, size);
        memcpy((void *)(block + 1), ptr, old_size);
    } else {
        struct heap_block *new_block = sbrk(real_size);
        if (!new_block) {
            return NULL;
        }
        new_block->size = size;
        new_block->next = NULL;
        new_block->free = 0;
        
        memcpy((void *)(new_block + 1), ptr, old_size);

        struct heap_block *last = block_list;
        while (last->next) {
            last = last->next;
        }
        last->next = new_block;

        block = new_block;
    }

    free(ptr);

    return block + 1;
}

