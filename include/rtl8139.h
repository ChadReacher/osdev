#ifndef RTL8139_H
#define RTL8139_H

#include <types.h>
#include <isr.h>

#define RTL8139_VENDOR_ID 0x10Ec
#define RTL8139_DEVICE_ID 0x8139

#define RX_BUFFER_SZ (8192 + 16 + 1500)

void rtl8139_init();
void rtl8139_handler(registers_state *regs);

#endif
