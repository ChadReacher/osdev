#include <tss.h>
#include <gdt.h>
#include <common.h>
#include <panic.h>

extern void tss_flush(void);

static volatile struct tss_entry kernel_tss;

// As we use the software multitasking, TSS is not used except for one case:
// when an interrupt occurs which leads to an increase of a privelege
// level (e.g. a system call).
// During the interrupt handling, the CPU reads the stack address(ESP0)
// and a stack segment selector(SS0) from the TSS and sets the
// corresponding stack registers: ESP and SS.
void tss_init(void) {
	u32 base = (u32)&kernel_tss;
	gdt_add_entry(base, base + sizeof(struct tss_entry), 0x89, 0x40);
	kernel_tss.ss0 = KERNEL_DS;
	kernel_tss.esp0 = 0x0;
	kernel_tss.cs = 0x0B;
	kernel_tss.ds = 0x13;
	kernel_tss.es = 0x13;
	kernel_tss.fs = 0x13;
	kernel_tss.gs = 0x13;
	kernel_tss.ss = 0x13;
	kernel_tss.iomap = sizeof(struct tss_entry);
	tss_flush();

    debug("TSS has been initialized\r\n");
}

void tss_set_stack(u32 kesp) {
	kernel_tss.esp0 = kesp;
}
