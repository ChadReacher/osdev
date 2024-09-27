#include <icmpv4.h>
#include <panic.h>
#include <ipv4.h>
#include <ethernet.h>
#include <string.h>
#include <heap.h>

extern u8 my_ip[4];

static u32 icmpv4_id = 1;

void icmpv4_receive_packet(u8 *payload, ipv4_header_t *ipv4_header) {
    struct icmpv4_header *icmpv4_echo = (struct icmpv4_header *)payload;

    debug("ICMPV4 packet\r\n");
    debug("type - 0x%x\r\n", icmpv4_echo->type);
    debug("code - 0x%x\r\n", icmpv4_echo->code);
    debug("checksum - 0x%x\r\n", icmpv4_echo->checksum);
    debug("id - 0x%x\r\n", icmpv4_echo->id);
    debug("sequence_number - 0x%x\r\n", icmpv4_echo->seq_num);

    if (icmpv4_echo->type == ICMPV4_TYPE_ECHO_REQUEST) {
        debug("Try to SEND ICMPV4 BACK\r\n");
        icmpv4_send_packet(ipv4_header->src_addr, icmpv4_echo);
    }
}

void icmpv4_send_packet(u32 dest_ip, struct icmpv4_header *echo_request) {
    u32 myip;
    struct icmpv4_header *icmpv4_header;

    debug("ICMPV4 sending packet...\r\n");
    if (echo_request) {
		icmpv4_header = malloc(sizeof(struct icmpv4_header) +
				sizeof(*echo_request->data)); 
        icmpv4_header->type = ICMPV4_TYPE_ECHO_REPLY;
        icmpv4_header->code = 0;
        icmpv4_header->checksum = 0;
        icmpv4_header->id = echo_request->id;
        icmpv4_header->seq_num = echo_request->seq_num;
		memcpy(icmpv4_header->data, echo_request->data, sizeof(*echo_request->data));
        icmpv4_header->checksum = ipv4_checksum(icmpv4_header,
				sizeof(struct icmpv4_header));
        myip = my_ip[0] | (my_ip[1] << 8) | (my_ip[2] << 16) | (my_ip[3] << 24);
        ipv4_send_packet((u8 *)icmpv4_header, sizeof(struct icmpv4_header), 0, 1,
				myip, dest_ip);
    } else {
	icmpv4_header = malloc(sizeof(struct icmpv4_header) + 6);
        icmpv4_header->type = ICMPV4_TYPE_ECHO_REQUEST;
        icmpv4_header->code = 0;
        icmpv4_header->checksum = 0;
        icmpv4_header->id = HTONS(icmpv4_id);
        icmpv4_header->seq_num = 1;
        icmpv4_header->data[0] = 'H';
        icmpv4_header->data[1] = 'e';
        icmpv4_header->data[2] = 'l';
        icmpv4_header->data[3] = 'l';
        icmpv4_header->data[4] = 'o';
        icmpv4_header->data[5] = '!';
        ++icmpv4_id;
        icmpv4_header->checksum = ipv4_checksum(icmpv4_header,
				sizeof(struct icmpv4_header) + 6);
        myip = my_ip[0] | (my_ip[1] << 8) | (my_ip[2] << 16) | (my_ip[3] << 24);
        ipv4_send_packet((u8 *)icmpv4_header, sizeof(struct icmpv4_header) + 6, 0, 1,
				myip, dest_ip);
    }
	free(icmpv4_header);
}
