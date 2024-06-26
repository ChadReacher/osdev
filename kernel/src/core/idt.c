#include <idt.h>
#include <panic.h>

struct idt_entry idt[IDT_ENTRIES];
volatile struct idtr idtr;

void idt_set(u8 index, u32 isr, u8 flags) {
	idt[index].isr_address_low = ((u32)isr & 0xFFFF);
	idt[index].kernel_cs = 0x08;
	idt[index].reserved = 0x00;
	idt[index].attributes = flags;
	idt[index].isr_address_high = ((u32)isr >> 16) & 0xFFFF;
}

void idt_init() {
	idtr.base = (u32) &idt;
	idtr.limit = IDT_ENTRIES * sizeof(struct idt_entry) - 1;
	__asm__ volatile ("lidt %0" : : "m" (idtr));
	debug("IDT has been initialized\r\n");
}
