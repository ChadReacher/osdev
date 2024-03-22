#include <idt.h>

idt_entry_t idt[IDT_ENTRIES];
idtr_t idtr;

void idt_set(u8 index, u32 isr, u8 flags) {
	idt[index].isr_address_low = ((u32)isr & 0xFFFF);
	idt[index].kernel_cs = 0x08;
	idt[index].reserved = 0x00;
	idt[index].attributes = flags;
	idt[index].isr_address_high = ((u32)isr >> 16) & 0xFFFF;
}

void idt_init() {
	idtr.base = (u32) &idt;
	idtr.limit = IDT_ENTRIES * sizeof(idt_entry_t) - 1;
	__asm__ __volatile__ ("lidt %0" : : "m" (idtr));
	debug("IDT has been initialized\r\n");
}
