#ifndef RTL8139_H
#define RTL8139_H

#include <types.h>
#include <isr.h>

#define RTL8139_VENDOR_ID 0x10Ec
#define RTL8139_DEVICE_ID 0x8139

#define RX_BUFFER_SZ 8192
#define TX_BUFFER_SZ 1518

#define RX_READ_POINTER_MASK (~3)
#define CMD_REG 0x37
#define CONF1_REG 0x52
#define RXBUF_REG 0x30
#define RXBUFPTR_REG 0x38
#define IMR_REG 0x3C
#define ISR_REG 0x3E
#define RX_CONF_REG 0x44
#define ID_REG0 0x0
#define ID_REG4 0x4

enum CmdRegBits {
    RxBufEmpty = 0x1,
    TxEnable = 0x4,
    RxEnable = 0x8,
    CmdReset = 0x10
};

enum IntrMaskBits {
    RxOk = 0x1,
    TxOk = 0x4
};

enum IntrStatusBits {
    ROK = 0x1,
    TOK = 0x4
};

enum RxConfBits {
    AcceptAllPhys = 0x1,
    AcceptMyPhys = 0x2,
    AcceptMulticast = 0x4,
    AcceptBroadcast = 0x8,
    DoWrap = 0x80
};

void rtl8139_init();
void rtl8139_transmit_data(void *data, u32 len);

#endif
