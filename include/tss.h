#ifndef TSS_H
#define TSS_H

#include "types.h"

typedef struct tss_entry {
	u32 prev_tss;
	u32 esp0;
	u32 ss0;
	u32 esp1;
	u32 ss1;
	u32 esp2;
	u32 ss2;
	u32 cr3;
	u32 eip;
	u32 eflags;
	u32 eax;
	u32 ecx;
	u32 edx;
	u32 ebx;
	u32 esp;
	u32 ebp;
	u32 esi;
	u32 edi;
	u32 es;
	u32 cs;
	u32 ss;
	u32 ds;
	u32 fs;
	u32 gs;
	u32 ldtr;
	u32 trap;
	u32 iomap;
} __attribute__((packed)) tss_entry_t;

extern void tss_flush();

void tss_init(u32 idx, u32 kss, u32 kesp);
void tss_set_stack2(u32 kesp);

#endif
