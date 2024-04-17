#include <tss.h>
#include <gdt.h>
#include <string.h>

volatile struct tss_entry kernel_tss;

void tss_init(u32 idx, u32 kss, u32 kesp) {
	u32 base = (u32)&kernel_tss;
	gdt_set_entry(idx, base, base + sizeof(struct tss_entry), 0x89, 0x4);
	kernel_tss.ss0 = kss;
	kernel_tss.esp0 = kesp;
	kernel_tss.cs = 0x0B;
	kernel_tss.ds = 0x13;
	kernel_tss.es = 0x13;
	kernel_tss.fs = 0x13;
	kernel_tss.gs = 0x13;
	kernel_tss.ss = 0x13;
	kernel_tss.iomap = sizeof(struct tss_entry);
	tss_flush();
}

void tss_set_stack(u32 kesp) {
	kernel_tss.esp0 = kesp;
}
