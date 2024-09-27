#ifndef TCP_H
#define TCP_H

#include <types.h>

struct tcp_header {
	u16 src_port;
	u16 dest_port;
	u32 seq_num;
	u32 ack_num;
	u16 doff_rsvd_bits;
	u16 window;
	u16 checksum;
	u16 urg_p;
};

void tcp_receive_fragment(void *data, u32 data_len);
void tcp_transmit_fragment();

#endif
