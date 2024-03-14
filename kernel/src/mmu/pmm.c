#include <pmm.h>
#include <string.h>
#include <serial.h>
#include <panic.h> 

u32 *memory_map = 0;
u32 total_blocks = 0;
u32 used_blocks = 0;

void set_block(u32 bit) {
	u32 idx, offset;
	
	idx = INDEX_FROM_BIT(bit);
	offset = OFFSET_FROM_BIT(bit);
	memory_map[idx] |= (1 << offset);
}

void clear_block(u32 bit) {
	u32 idx, offset;
	
	idx = INDEX_FROM_BIT(bit);
	offset = OFFSET_FROM_BIT(bit);
	memory_map[idx] &= ~(1 << offset);
}

u8 test_block(u32 bit) {
	u32 idx, offset;
	
	idx = INDEX_FROM_BIT(bit);
	offset = OFFSET_FROM_BIT(bit);
	return memory_map[idx] & (1 << offset);
}

i32 find_first_free_blocks(u32 num_blocks) {
	u32 memory_bitmap, next_memory_bitmap;
	u32 i, j, count;
	if (num_blocks == 0) return -1;

	/*
	Iterate through integers(4 bytes) that contain blocks.
	Each integer contains 4 * BLOCKS_PER_BYTE(8) = 32 blocks.
	Number of these integers equal 'total_blocks / 32'.
	*/
	for (i = 0; i < total_blocks / 32; ++i) { 
		memory_bitmap = memory_map[i];
		/* Check if this integer is fully used? */
		if (memory_bitmap != 0xFFFFFFFF) { 
			/* Iterate through each block in the integer */
			for (j = 0; j < 32; ++j) {
				u32 bit = 1 << j;
				/* Check if the block 'j' in integer 'memory_map[i]' is not used */
				if ((memory_bitmap & bit) == 0) { 
					/*u32 start_bit = i * 32 + bit; Get bit at index 'i' within memory map */
					u32 found_free_blocks = 0;
					for (count = 0, found_free_blocks = 0; count < num_blocks; ++count) {
						/* If we are at the end of the current bitmap and there  */
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

						if (found_free_blocks == num_blocks) {
							return i * 32 + j;
						}
					}
				}
			}
		}
	}
	return -1;
}
void pmm_init() {
	u32 num_mmap_entries;
	memory_map_entry *mmap_entry;
	u32 total_memory_bytes;
	u32 i;

	/* Get number of memory map entries */
	num_mmap_entries = *((u32 *)BIOS_NUM_ENTRIES);
	/* Get starting address of entries list */
	mmap_entry = (memory_map_entry *)BIOS_MEMORY_MAP;
	mmap_entry += num_mmap_entries - 1;
	/* Get total amount of bytes */
	total_memory_bytes = mmap_entry->base_address_low + mmap_entry->length_low - 1;

	/* Initialize physical memory manager at 0x30000  */
	/* to all available memory. By default all memory is used/reserved. */
	_pmm_init(0xC0030000, total_memory_bytes); 

	/* Get back to start of the list to available memory as free to use */
	mmap_entry = (memory_map_entry *)BIOS_MEMORY_MAP;
	for (i = 0; i < num_mmap_entries; ++i) {
		if (mmap_entry->type == 1) { /* If the type of memory chunk is 'Available Memory'? */
			mark_memory_as_free(mmap_entry->base_address_low, mmap_entry->length_low);
		}
		++mmap_entry;
	}

	mark_memory_as_used(0xA000, 0x800);	/* Mark font as used memory */
	
	/* mark kernel and "OS" memory regions as used */
	mark_memory_as_used(0x10000, 0x25000);
	
	/* Mark physical memory map itself as used */
	mark_memory_as_used(30000, total_blocks / BLOCKS_PER_BYTE);
	
	mark_memory_as_used(0x26000, 1);

	debug("Physical memory manager has been initialized\r\n");
	debug("Number of used or reserved 4K blocks: %x\r\n", used_blocks);
	debug("Number of free 4K blocks: %x\r\n", (total_blocks - used_blocks));
	debug("Total amount of 4K blocks: %x\r\n", total_blocks);
	print_physical_memory_info();
	debug("total_blocks - 0x%p\r\n", &total_blocks);
	debug("used_blocks - 0x%p\r\n", &used_blocks);
	debug("memory_map - 0x%p\r\n", &memory_map);
}

void _pmm_init(u32 start_address, u32 size) {
	memory_map = (u32 *)start_address;
	total_blocks = size / PMM_BLOCK_SIZE;
	used_blocks = total_blocks;

	memset(memory_map, 0xFF, total_blocks / BLOCKS_PER_BYTE);
}

void mark_memory_as_free(u32 base_address, u32 size) {
	u32 align = base_address / PMM_BLOCK_SIZE;
	u32 num_blocks = size / PMM_BLOCK_SIZE;
	for (; num_blocks > 0; --num_blocks) {
		clear_block(align++);
		--used_blocks;
	}
	/* Insure that we won't overwrite Bios Data Area / IVT */
	set_block(0); 
}

void mark_memory_as_used(u32 base_address, u32 size) {
	u32 align = base_address / PMM_BLOCK_SIZE;
	u32 num_blocks = size / PMM_BLOCK_SIZE;
	if (size % PMM_BLOCK_SIZE > 0) {
		++num_blocks;
	}
	for (; num_blocks > 0; --num_blocks) {
		set_block(align++);
		++used_blocks;
	}
}

void *allocate_blocks(u32 num_blocks) {
	u32 i;
	i32 starting_block;
	u32 address;
	if (num_blocks >= (total_blocks - used_blocks)) 
		return 0;

	/* Find free blocks */
	starting_block = find_first_free_blocks(num_blocks);
	if (starting_block == -1) 
		return 0;

	/* Mark them as used */
	for (i = 0; i < num_blocks; ++i)
		set_block(starting_block + i);
	used_blocks += num_blocks;

	/* Return its address */
	address = starting_block * PMM_BLOCK_SIZE;
	return (void *)address;
}

void free_blocks(void *address, u32 num_blocks) {
	u32 i;
	u32 starting_block = (u32)address / PMM_BLOCK_SIZE;
	for (i = 0; i < num_blocks; ++i) {
		clear_block(starting_block + i);
	}
	used_blocks -= num_blocks;
}

void print_physical_memory_info() {
	u8 i;
	u32 num_entries;
	memory_map_entry *mmap_entry;

	mmap_entry = (memory_map_entry *)BIOS_MEMORY_MAP;
	num_entries = *((u32 *)BIOS_NUM_ENTRIES);
	debug("Physical memory info: \r\n");
	debug("Total number of entries: %d\r\n", num_entries);
	for (i = 0; i < num_entries; ++i) {
		serial_printf("Region: %x | Base: %x | Length: %x | Type(%d): ", 
				i, mmap_entry->base_address_low, 
				mmap_entry->length_low, mmap_entry->type);
		switch (mmap_entry->type) {
			case 1:
				serial_printf("Available Memory");
				break;
			case 2:
				serial_printf("Reserved Memory");
				break;
			case 3:
				serial_printf("ACPI Reclaim Memory");
				break;
			case 4:
				serial_printf("ACPI NVS Memory");
				break;
			default:
				serial_printf("Undefined Memory");
				break;
		}
		serial_printf("\r\n");
		++mmap_entry; 
	}
	--mmap_entry;
	debug("Total amount of memory(in bytes): %x\r\n", mmap_entry->base_address_low + mmap_entry->length_low - 1);
}
