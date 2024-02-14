#ifndef PIC_H
#define PIC_H

#include "types.h"

#define PIC1_CMD  0x20
#define PIC1_DATA 0x21

#define PIC2_CMD  0xA0
#define PIC2_DATA 0xA1

#define NEW_IRQ_START_PRIMARY  0x20 // IRQ 0-7 => 0x20-0x27
#define NEW_IRQ_START_SLAVE  0x28 // IRQ 8-15 => 0x28-0x2F

#define PIC_EOI    0x20 // End-of-interrupt command code

void pic_disable();
void set_irq_mask(u8 irq_line);
void clear_irq_mask(u8 irq_line);
void pic_remap();
void pic_send_eoi(u8 irq);

#endif 
