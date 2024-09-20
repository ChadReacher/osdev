#ifndef ETHERNET_H
#define ETHERNET_H

#include <types.h>

#define NTOHS(n) ((((n) & 0xFF) << 8) | (((n) & 0xFF00) >> 8))
#define HTONS(n) NTOHS(n)

#define ETHER_TYPE_IP    0x0800
#define ETHER_TYPE_ARP   0x0806

#define ETHER_MIN_PAYLOAD_LEN 46
#define ETHER_MAX_PAYLOAD_LEN 1500
#define ETHER_MAX_FRAME_LEN 1514

struct ethernet_header {
    u8 dest_mac[6];
    u8 src_mac[6];
    u16 ethernet_type;
} __attribute__((packed));

void ethernet_receive_frame(void *data, u32 data_len);
i32 ethernet_send_frame(u8 *dest_mac, u16 ethernet_type, u8 *data, u32 len);

#endif
