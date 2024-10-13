#include <icmpv4.h>
#include <panic.h>
#include <ipv4.h>
#include <ethernet.h>
#include <string.h>
#include <heap.h>

extern u8 my_ip[4];

static u32 icmpv4_id = 1;

static void icmpv4_send_reply_packet(u32 dest_ip, u8 *packet, u32 packet_len);

void icmpv4_receive_packet(u8 *packet, u32 packet_len, ipv4_header_t *ipv4_header) {
    struct icmpv4_header *icmpv4_echo = (struct icmpv4_header *)packet;

    debug("ICMPV4 packet\r\n");
    debug("type - 0x%x\r\n", icmpv4_echo->type);
    debug("code - 0x%x\r\n", icmpv4_echo->code);
    debug("checksum - 0x%x\r\n", icmpv4_echo->checksum);
    debug("id - 0x%x\r\n", icmpv4_echo->id);
    debug("sequence_number - 0x%x\r\n", icmpv4_echo->seq_num);

    if (icmpv4_echo->type == ICMPV4_TYPE_ECHO_REQUEST) {
        debug("Try to SEND ICMPV4 BACK\r\n");
        icmpv4_send_reply_packet(ipv4_header->src_addr, packet, packet_len);
    }
}

static void icmpv4_send_reply_packet(u32 dest_ip, u8 *orig_packet, u32 packet_len) {
    struct icmpv4_header reply;
    struct icmpv4_header *echo_request = (struct icmpv4_header *)orig_packet;
    u32 payload_len = packet_len - sizeof(struct icmpv4_header);

    reply.type = ICMPV4_TYPE_ECHO_REPLY;
    reply.checksum = 0;
    reply.id = echo_request->id;
    reply.seq_num = echo_request->seq_num;
    reply.code = 0;
    reply.checksum = ipv4_checksum(&reply, sizeof(struct icmpv4_header));

    u8 *packet = malloc(sizeof(struct icmpv4_header) + payload_len);
    memcpy(packet, &reply, sizeof(struct icmpv4_header));
    memcpy(packet + sizeof(struct icmpv4_header), orig_packet + sizeof(struct icmpv4_header), payload_len);
    ipv4_send_packet(packet, sizeof(struct icmpv4_header) + payload_len, IPV4_PROTO_ICMP, dest_ip);
    free(packet);
}

void icmpv4_send_packet(u32 dest_ip) {
    struct icmpv4_header icmpv4_header;

    debug("ICMPV4 sending packet...\r\n");
    icmpv4_header.type = ICMPV4_TYPE_ECHO_REQUEST;
    icmpv4_header.code = 0;
    icmpv4_header.checksum = 0;
    icmpv4_header.id = HTONS(icmpv4_id);
    icmpv4_header.seq_num = 1;
    ++icmpv4_id;

    u8 data[6];
    data[0] = 'H'; data[1] = 'e'; data[2] = 'l'; data[3] = 'l'; data[4] = 'o'; data[5] = '!';
    u8 *packet = malloc(sizeof(struct icmpv4_header) + 6);
    memcpy(packet, &icmpv4_header, sizeof(struct icmpv4_header));
    memcpy(packet + sizeof(struct icmpv4_header), data, 6);

    icmpv4_header.checksum = ipv4_checksum(packet, sizeof(struct icmpv4_header) + 6);
    memcpy(packet, &icmpv4_header, sizeof(struct icmpv4_header));

    ipv4_send_packet(packet, sizeof(struct icmpv4_header) + 6, IPV4_PROTO_ICMP, dest_ip);
    free(packet);
}
