#ifndef IDT_H
#define IDT_H

#include "types.h"

#define IDT_ENTRIES 256
#define INTERRUPT_GATE_TYPE 0x8E /* 10001110 */
/* 
 * 1 bit		P(present bit),
 * 2-3 bits 	DPL(Descriptor Privelege Level)
 * 4 bit		Default(0),
 * 5 bit		D(size of gate, 1 - 32 bits)
 * 6-8 bits 	Interrupt gate(110) or Trap Gate(111)
 */

typedef struct {
	u16 isr_address_low;	/* The lower 16 bits of the ISR's address */
	u16 kernel_cs;		 	/* Code segment for this ISR */
	u8 reserved;		 	/* Set to 0 */
	u8 attributes;		 	/* Type and attributes */
	u16 isr_address_high;	/* The higher 16 bits of the ISR's address */
} __attribute__((packed)) idt_entry_t;

typedef struct {
	u16 limit;
	u32 base;
} __attribute__((packed)) idtr_t;

void idt_set(u8 index, u32 isr, u8 flags);
void init_idt();

#endif
