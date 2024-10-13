#ifndef IPV4_H
#define IPV4_H

#include <types.h>

#define IPV4_FLAGS_MF (1 << 0)
#define IPV4_FLAGS_DF (1 << 1)

#define IPV4_PROTO_ICMP 1
#define IPV4_PROTO_IGMP 2
#define IPV4_PROTO_TCP  6
#define IPV4_PROTO_UDP  17

typedef struct ipv4_header {
    u8 ihl_and_version;
    u8 service;
    u16 length;
    u16 id;
    u16 flags;
    u8 ttl;
    u8 proto;
    u16 checksum;
    u32 src_addr;
    u32 dest_addr;
} __attribute__((packed)) ipv4_header_t;

void ipv4_receive_packet(void *packet);
void ipv4_send_packet(u8 *data, u32 len, u8 protocol, u32 dest_addr);
u16 ipv4_checksum(void *addr, i32 count);

#endif
