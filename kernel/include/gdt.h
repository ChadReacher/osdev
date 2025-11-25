#ifndef GDT_H
#define GDT_H

#include "types.h"

void gdt_init(void);
void gdt_add_entry(u32 base, u32 limit, u8 access_byte, u8 granularity);

#endif
