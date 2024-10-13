#ifndef ICMPV4_H
#define ICMPV4_H

#include <types.h>
#include <ipv4.h>

#define ICMPV4_TYPE_ECHO_REPLY      0
#define ICMPV4_TYPE_ECHO_REQUEST    8

struct icmpv4_header {
    u8 type;
    u8 code;
    u16 checksum;
    u16 id;
    u16 seq_num;
} __attribute__((packed));

void icmpv4_receive_packet(u8 *packet, u32 plen, ipv4_header_t *ipv4_header);
void icmpv4_send_packet(u32 dest_ip);

#endif
