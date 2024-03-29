#ifndef BIOS_MEMORY_MAP_H
#define BIOS_MEMORY_MAP_H

#include "types.h"

#define BIOS_NUM_ENTRIES 0xC0008500
#define BIOS_MEMORY_MAP 0xC0008504

typedef struct {
	u64 base_address;
	u64 length;
	u32 type;
	u32 acpi;
} __attribute__((packed)) memory_map_entry;

#endif
