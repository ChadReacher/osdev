#include <gdt.h>
#include <panic.h>

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

static struct gdt_entry gdt[GDT_ENTRIES];
static volatile struct gdtr gdtr;

void gdt_init(void) {
    gdtr.size = sizeof(gdt) - 1;
    gdtr.offset = (u32)gdt;

    /* NULL descriptor */
    gdt_add_entry(0, 0, 0, 0);

    /* Kernel code descriptor */
    gdt_add_entry(0x0, 0xFFFFFFFF, 0x9A, 0xCF);

    /* Kernel data descriptor */
    gdt_add_entry(0x0, 0xFFFFFFFF, 0x92, 0xCF);

    /* User code descriptor */
    gdt_add_entry(0x0, 0xFFFFFFFF, 0xFA, 0xCF);

    /* User data descriptor */
    gdt_add_entry(0x0, 0xFFFFFFFF, 0xF2, 0xCF);

    gdt_flush((u32)(&gdtr));
}

void gdt_add_entry(u32 base, u32 limit, u8 access_byte, u8 granularity) {
    static u32 idx = 0;
    assert(idx <= GDT_ENTRIES);

    struct gdt_entry *entry = &gdt[idx++];
    entry->base_low = base & 0xFFFF;
    entry->base_middle = ((base >> 16) & 0xFF);
    entry->base_high = ((base >> 24) & 0xFF);
    entry->limit_low = limit & 0xFFFF;
    entry->granularity = (limit >> 16) & 0x0F;
    entry->access_byte = access_byte;
    entry->granularity |= (granularity & 0xF0);
}
