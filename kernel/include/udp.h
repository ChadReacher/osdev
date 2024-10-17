#ifndef UDP_H
#define UDP_H

#include <types.h>

struct udp_header {
	u16 src_port;
	u16 dest_port;
	u16 length;
	u16 checksum;
} __attribute__((packed));

void udp_receive_datagram(void *datagram, u32 datagram_len);
void udp_transmit_datagram();

#endif
