#ifndef IDT_H
#define IDT_H

#include "types.h"

#define IDT_ENTRIES 256
/* 1110 1110 */
#define INTERRUPT_GATE_TYPE_KERNEL 0xEE
#define INTERRUPT_GATE_TYPE 0x8E
/* 10001110
 * 1 bit        P(present bit),
 * 2-3 bits     DPL(Descriptor Privelege Level)
 * 4 bit        Default(0),
 * 5 bit        D(size of gate, 0 - 16 bits or task gate, 1 - 32 bits)
 * 6-8 bits     Interrupt gate(110) or Trap Gate(111)
 */

struct idt_entry {
    u16 isr_address_low;    /* The lower 16 bits of the ISR's address */
    u16 kernel_cs;          /* Code segment for this ISR */
    u8 reserved;            /* Set to 0 */
    /* 1st bit: Present
     * 2-3rd bits: DPL (Descriptor Privelege Level)
     * 4th bit: zero
     * 5-8th bits: gate type: task gate, 16/32 bit interrupt/trap gate
     */
    u8 attributes;
    u16 isr_address_high;   /* The higher 16 bits of the ISR's address */
} __attribute__((packed));

struct idtr {
    u16 limit;
    u32 base;
} __attribute__((packed));

void idt_set(u8 index, u32 isr, u8 flags);
void idt_init(void);

#endif
