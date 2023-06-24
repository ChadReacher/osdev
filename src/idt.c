#include "idt.h"

void idt_set(u8 index, void *isr, u8 flags) {
	idt[index].isr_address_low = (u32)isr & 0xFFFF;
	idt[index].kernel_cs = 0x08;
	idt[index].reserved = 0x00;
	idt[index].attributes = flags;
	idt[index].isr_address_high = ((u32)isr >> 16) & 0xFFFF;
}

void init_idt() {
	idtr.base = (u32)&idt;
	idtr.limit = IDT_ENTRIES * sizeof(idt_entry_t) - 1;
	asm volatile ("lidt (%0)" : : "r" (&idtr));
}
