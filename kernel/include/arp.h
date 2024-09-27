#ifndef ARP_H
#define ARP_H

#include <types.h>

#define ARP_REQUEST 0x1
#define ARP_REPLY   0x2

struct arp_packet {
    u16 hardware_type;
    u16 protocol_type;
    u8 hardware_addr_len;
    u8 protocol_addr_len;
    u16 operation;
    u8 src_mac[6];
    u8 src_ip[4];
    u8 dest_mac[6];
    u8 dest_ip[4];
} __attribute__((packed));

void arp_receive_packet(void *packet);
void arp_send_packet(u8 *dst_ip);
u32 arp_lookup(u8 *dest_ip, u8 *dest_mac);

#endif
