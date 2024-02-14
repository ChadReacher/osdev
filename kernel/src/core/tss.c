#include <tss.h>
#include <gdt.h>
#include <string.h>

tss_entry_t kernel_tss;

void tss_init(u32 idx, u32 kss, u32 kesp) {
	u32 base = (u32)&kernel_tss;
	gdt_set_entry(idx, base, base + sizeof(tss_entry_t), 0xE9, 0);
	memset(&kernel_tss, 0, sizeof(tss_entry_t));
	kernel_tss.ss0 = kss;
	kernel_tss.esp0 = kesp;
	kernel_tss.cs = 0x0B;
	kernel_tss.ds = 0x13;
	kernel_tss.es = 0x13;
	kernel_tss.fs = 0x13;
	kernel_tss.gs = 0x13;
	kernel_tss.ss = 0x13;
	kernel_tss.iomap = sizeof(tss_entry_t);
	tss_flush();
}

void tss_set_stack(u32 kesp) {
	kernel_tss.esp0 = kesp;
}
