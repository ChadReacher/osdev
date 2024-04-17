#ifndef BIOS_MEMORY_MAP_H
#define BIOS_MEMORY_MAP_H

#include "types.h"

#define BIOS_NUM_ENTRIES 0xC0008500
#define BIOS_MEMORY_MAP 0xC0008504

struct phys_mmap_entry {
	u32 base_address_low;
	u32 base_address_high;
	u32 length_low;
	u32 length_high;
	u32 type;
	u32 acpi;
} __attribute__((packed));

#endif
