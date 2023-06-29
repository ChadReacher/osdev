#ifndef BIOS_MEMORY_MAP_H
#define BIOS_MEMORY_MAP_H

typedef struct {
	u64 base_address;
	u64 length;
	u32 type;
	u32 acpi;
} __attribute__((packed)) memory_map_entry;


memory_map_entry *bios_memory_map = (memory_map_entry *)0x8504;

#endif
