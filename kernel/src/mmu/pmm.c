#include <pmm.h>
#include <bios_memory_map.h>
#include <panic.h> 
#include <serial.h>
#include <string.h>
#include <common.h>

#define MEMORY_BITMAP_PHYS 0x70000
#define BLOCKS_PER_BYTE 8

#define INDEX_FROM_BIT(b) (b / 32)
#define OFFSET_FROM_BIT(b) (b % 32)

extern u32 _text_start_;
extern u32 _text_end_;
extern u32 _rodata_start_;
extern u32 _rodata_end_;
extern u32 _data_start_;
extern u32 _data_end_;
extern u32 _bss_start_;
extern u32 _bss_end_;

static u32 *memory_map = 0;
static u32 total_blocks = 0;
static u32 used_blocks = 0;

static void _pmm_init(u32 start_address, u32 size);
static void set_block(u32 bit);
static void clear_block(u32 bit);
static u8 test_block(u32 bit);
static i32 find_first_free_blocks(u32 num_blocks);
static void mark_memory_as_free(u32 base_address, u32 size);
static void mark_memory_as_used(u32 base_address, u32 size);
static void phys_mem_dump(void);
static void kernel_layout_dump(void);

static void set_block(u32 bit) {
    const u32 idx = INDEX_FROM_BIT(bit);
    const u32 offset = OFFSET_FROM_BIT(bit);
    memory_map[idx] |= (1 << offset);
}

static void clear_block(u32 bit) {
    const u32 idx = INDEX_FROM_BIT(bit);
    const u32 offset = OFFSET_FROM_BIT(bit);
    memory_map[idx] &= ~(1 << offset);
}

static u8 test_block(u32 bit) {
    const u32 idx = INDEX_FROM_BIT(bit);
    const u32 offset = OFFSET_FROM_BIT(bit);
    return (memory_map[idx] & (1 << offset)) == (u32)(1 << offset);
}

static i32 find_first_free_blocks(u32 num_blocks) {
    u32 next_memory_bitmap;
    if (num_blocks == 0) return -1;

    /*
    Iterate through integers(4 bytes) that contain blocks.
    Each integer contains 4 * BLOCKS_PER_BYTE(8) = 32 blocks.
    Number of these integers equal 'total_blocks / 32'.
    */
    for (u32 i = 0; i < total_blocks / 32; ++i) {
        u32 memory_bitmap = memory_map[i];
        /* Check if this integer is fully used? */
        if (memory_bitmap == 0xFFFFFFFF) {
            continue;
        }
        /* Iterate through each block in the integer */
        for (u32 j = 0; j < 32; ++j) {
            u32 bit = 1 << j;
            /* Check if the block 'j' in integer 'memory_map[i]' is not used */
            if ((memory_bitmap & bit) != 0) {
                continue;
            }
            /*u32 start_bit = i * 32 + bit; Get bit at index 'i' within memory map */
            u32 found_free_blocks = 0;
            for (u32 count = 0; count < num_blocks; ++count) {
                /* If we are at the end of the current bitmap and there */
                /* is free blocks in the next bitmap, also we are not at the */
                /* end of memory */
                if ((j + count > 31) && (i + 1 <= total_blocks / 32)) {
                    next_memory_bitmap = memory_map[i + 1];
                    if ((next_memory_bitmap & (1 << ((j + count) - 32))) == 0) {
                        ++found_free_blocks;
                    }
                } else {
                    if (!(memory_bitmap & (1 << (j + count)))) {
                        ++found_free_blocks;
                    }
                }

                /* If we reached the desired number of frames
                 * just return */
                if (found_free_blocks == num_blocks) {
                    return i * 32 + j;
                }
            }
        }
    }
    return -1;
}

void pmm_init(void) {
    const struct phys_mmap_entry *mmap_entry;

    /* Get number of memory map entries */
    const u32 num_mmap_entries = *((u32 *)BIOS_NUM_ENTRIES);
    /* Get starting address of entries list */
    mmap_entry = (const struct phys_mmap_entry *)BIOS_MEMORY_MAP;
    mmap_entry += num_mmap_entries - 1;
    /* Get total amount of bytes */
    const u32 total_memory_bytes = mmap_entry->base_address_low + mmap_entry->length_low - 1;

    /* Initialize physical memory manager at the memory_bitmap_vaddr */
    /* to all available memory. By default all memory is used/reserved. */
    u32 memory_bitmap_vaddr = KERNEL_LOAD_VADDR + MEMORY_BITMAP_PHYS;
    _pmm_init(memory_bitmap_vaddr, total_memory_bytes);

    u32 initial_ram = 0;
    /* Get back to start of the list to available memory as free to use */
    mmap_entry = (struct phys_mmap_entry *)BIOS_MEMORY_MAP;
    for (u32 i = 0; i < num_mmap_entries; ++i) {
        /* If the type of memory chunk is 'Available Memory'? */
        if (mmap_entry->type == 1) {
            mark_memory_as_free(mmap_entry->base_address_low, mmap_entry->length_low);
            initial_ram += mmap_entry->length_low;
        }
        ++mmap_entry;
    }

    u32 ram_mb = initial_ram / (1 * MIB);
    debug("Initial RAM size = %d MiB (%#x bytes)\r\n", ram_mb, initial_ram);

    kernel_layout_dump();

    const u32 kstart = (u32)(&_text_start_);
    const u32 kend = (u32)(&_bss_end_);
    const u32 ksize = kend - kstart;

    // Mark the "low memory" (below 1 MiB) as "used"
    mark_memory_as_used(0x0, 1 * MIB);
    // Mark the kernel memory region as "used"
    mark_memory_as_used((u32)&_text_start_ - KERNEL_LOAD_VADDR, ksize);
    // Mark the physical memory map itself as "used"
    // Should be already marked as used during previous (low memory marking) step
    mark_memory_as_used(MEMORY_BITMAP_PHYS, total_blocks / BLOCKS_PER_BYTE);

    phys_mem_dump();

    debug("Physical memory manager has been initialized\r\n");
}

static void _pmm_init(u32 start_address, u32 size) {
    memory_map = (u32 *)start_address;
    total_blocks = size / PMM_BLOCK_SIZE;
    used_blocks = total_blocks;

    memset(memory_map, 0xFF, total_blocks / BLOCKS_PER_BYTE);
}

static void mark_memory_as_free(u32 base_address, u32 size) {
    assert(size != 0);

    u32 align = base_address / PMM_BLOCK_SIZE;
    u32 num_blocks = size / PMM_BLOCK_SIZE;
    if (size % PMM_BLOCK_SIZE > 0) {
        ++num_blocks;
    }
    for (u32 i = 0; i < num_blocks; ++i) {
        clear_block(align++);
        --used_blocks;
    }
}

static void mark_memory_as_used(u32 base_address, u32 size) {
    assert(size != 0);

    u32 align = base_address / PMM_BLOCK_SIZE;
    u32 num_blocks = size / PMM_BLOCK_SIZE;
    if (size % PMM_BLOCK_SIZE > 0) {
        ++num_blocks;
    }
    for (u32 i = 0; i < num_blocks; ++i) {
        set_block(align++);
        ++used_blocks;
    }
}

// Allocates `num_blocks` contigious physical pages 4K size each
void *allocate_blocks(u32 num_blocks) {
    assert(num_blocks != 0);

    if (num_blocks >= (total_blocks - used_blocks)) {
        return NULL;
    }

    /* Find free blocks */
    const i32 starting_block = find_first_free_blocks(num_blocks);
    if (starting_block == -1) {
        return NULL;
    }

    /* Mark them as used */
    for (u32 i = 0; i < num_blocks; ++i) {
        assert(test_block(starting_block + i) == 0);
        set_block(starting_block + i);
    }
    used_blocks += num_blocks;

    /* Return its address */
    const u32 address = starting_block * PMM_BLOCK_SIZE;
    return (void *)address;
}

void free_blocks(void *address, u32 num_blocks) {
    assert(address != NULL);
    assert(num_blocks != 0);

    u32 starting_block = (u32)address / PMM_BLOCK_SIZE;
    for (u32 i = 0; i < num_blocks; ++i) {
        assert(test_block(starting_block + i) != 0);
        clear_block(starting_block + i);
    }
    used_blocks -= num_blocks;
}

static void phys_mem_dump(void) {
    debug("Physical memory info: \r\n");
    debug("Used pages - %#x\r\n", used_blocks);
    debug("Free pages - %#x\r\n", (total_blocks - used_blocks));

    const char *bios_mem_type[] = {NULL,           "Available", "Reserved",
                                   "ACPI Reclaim", "ACPI NVS",  "Undefined",
                                   "Disabled"};
    struct phys_mmap_entry *entry = (struct phys_mmap_entry *)BIOS_MEMORY_MAP;
    const u32 num_entries = *((u32 *)BIOS_NUM_ENTRIES);
    debug("BIOS memory map:\r\n");
    debug("Region | Base       | Length     | Type\r\n");
    for (u32 i = 0; i < num_entries; ++i) {
        debug("#%d     | %#010x | %#010x | %s\r\n",
              i, entry->base_address_low,
              entry->length_low, bios_mem_type[entry->type]);
        ++entry;
    }
}

static void kernel_layout_dump(void) {
    const u32 kstart = (u32)(&_text_start_);
    const u32 kend = (u32)(&_bss_end_);
    const u32 ksize = kend - kstart;
    debug("Section |    Start   |     End    |\r\n"
          ".text   | %#x | %#x |\r\n"
          ".rodata | %#x | %#x |\r\n"
          ".data   | %#x | %#x |\r\n"
          ".bss    | %#x | %#x |\r\n",
          &_text_start_, &_text_end_, &_rodata_start_, &_rodata_end_,
          &_data_start_, &_data_end_, &_bss_start_, &_bss_end_
          );
    debug("Kernel size (w/ alignment)  = %#x\r\n", ksize);
    const u32 text_size = (u32)&_text_end_ - (u32)&_text_start_;
    const u32 rodata_size = (u32)&_rodata_end_ - (u32)&_rodata_start_;
    const u32 data_size = (u32)&_data_end_ - (u32)&_data_start_;
    const u32 bss_size = (u32)&_bss_end_ - (u32)&_bss_start_;
    const u32 ksize2 = text_size + rodata_size + data_size + bss_size;
    debug("Kernel size (w/o alignment) = %#x\r\n", ksize2);
}
