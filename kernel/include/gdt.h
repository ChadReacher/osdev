#ifndef GDT_H
#define GDT_H

#include "types.h"

#define GDT_ENTRIES 8

struct gdt_entry {
	u16 limit_low;
	u16 base_low;
	u8 base_middle;
	u8 access_byte;
	u8 granularity;
	u8 base_high;
} __attribute__((packed));

struct gdtr {
	u16 size;
	u32 offset;
} __attribute__((packed));

extern void gdt_flush(u32 gdtr);

void gdt_init();
void gdt_set_entry(u32 idx, u32 base, u32 limit, u8 access_byte, u8 granularity);

#endif
