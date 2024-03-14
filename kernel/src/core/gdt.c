#include <gdt.h>

gdt_entry_t gdt[GDT_ENTRIES];
gdtr_t gdtr;

void gdt_init() {
	gdtr.size = sizeof(gdt) - 1;
	gdtr.offset = (u32)gdt;

	/* NULL descriptor */
	gdt_set_entry(0, 0, 0, 0, 0);
	
	/* Kernel code descriptor */
	gdt_set_entry(1, 0, 0xFFFFFFFF, 0x9A, 0xCF);
	
	/* Kernel data descriptor */
	gdt_set_entry(2, 0, 0xFFFFFFFF, 0x92, 0xCF);

	/* User code descriptor */
	gdt_set_entry(3, 0, 0xFFFFFFFF, 0xFA, 0xCF);

	/* User data descriptor */
	gdt_set_entry(4, 0, 0xFFFFFFFF, 0xF2, 0xCF);

	gdt_flush((u32)(&gdtr));
}

void gdt_set_entry(u32 idx, u32 base, u32 limit, u8 access_byte, u8 granularity) {
	gdt_entry_t *entry = &gdt[idx];
	entry->base_low = base & 0xFFFF;
	entry->base_middle = ((base >> 16) & 0xFF);
	entry->base_high = ((base >> 24) & 0xFF);
	entry->limit_low = limit & 0xFFFF;
	entry->granularity = (limit >> 16) & 0x0F;
	entry->access_byte = access_byte;
	entry->granularity |= (granularity & 0xF0);
}
