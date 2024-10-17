#include <ipv4.h>
#include <panic.h>
#include <ethernet.h>
#include <arp.h>
#include <icmpv4.h>
#include <heap.h>
#include <string.h>

#define MAX_FRAGMENT_SESSIONS 12

u32 netmask = 0x00FFFFFF;

struct IPFragment;

struct IPFragment {
    u16 id;
    u8 *buf;
    u32 size;
    u32 offset;
    struct IPFragment *next;
};

/* Support for now only `MAX_FRAGMENT_SESSIONS` unique simultaneous fragmentation processes */
static struct IPFragment fragments[MAX_FRAGMENT_SESSIONS] = {0};
static u32 idx = 0;

void create_first_fragment(u16 id, void *payload, u16 plen, u32 offset) {
    if (idx > MAX_FRAGMENT_SESSIONS) {
        debug("We support only %d unique fragmentation processes\r\n", MAX_FRAGMENT_SESSIONS);
        return;
    }
    struct IPFragment *fragment = &fragments[idx]; 
    idx = (idx + 1) % MAX_FRAGMENT_SESSIONS;
    fragment->id = id;
    fragment->buf = malloc(plen);
    memcpy(fragment->buf, payload, plen);
    fragment->size = plen;
    fragment->offset = offset;
    fragment->next = NULL;
}

void add_fragment(u16 id, void *payload, u16 plen, u32 offset) {
    u32 i;
    for (i = 0; i < MAX_FRAGMENT_SESSIONS; ++i) {
        if (fragments[i].id == id) {
            break;
        }
    }
    if (i == MAX_FRAGMENT_SESSIONS) {
        debug("Possible ERROR: couldn't add a fragment with id - 0x%x\r\n", id);
	return;
    }
    struct IPFragment *new = malloc(sizeof(struct IPFragment));
    new->id = id;
    new->buf = malloc(plen);
    memcpy(new->buf, payload, plen);
    new->size = plen;
    new->offset = offset;
    new->next = NULL;
    
    struct IPFragment *curr = &fragments[i];
    while (curr->next != NULL) {
        curr = curr->next;
    }
    curr->next = new;	
}

void *compose_payload(u16 id, u32 *payload_len) {
    u32 i, total_size = 0;
    for (i = 0; i < MAX_FRAGMENT_SESSIONS; ++i) {
        if (fragments[i].id == id) {
            break;
        }
    }
    if (i == MAX_FRAGMENT_SESSIONS) {
        debug("Possible ERROR: couldn't add a fragment with id - 0x%x\r\n", id);
	return NULL;
    }
    struct IPFragment *curr = &fragments[i];
    while (curr != NULL) {
        total_size += curr->size;
        curr = curr->next;
    }
    *payload_len = total_size;
    u8 *buf = malloc(total_size);
    curr = &fragments[i];
    while (curr != NULL) {
        memcpy(buf + curr->offset, curr->buf, curr->size);
        free(curr->buf);
        curr = curr->next;
    }
    
    /* Free things */
    curr = fragments[i].next;
    struct IPFragment *next;
    while (curr != NULL) {
        next = curr->next;
        free(curr);
        curr = next;
    }
    memset(&fragments[i], 0, sizeof(struct IPFragment));

    return buf;
}

static u32 ipv4_id = 1;

extern u8 my_ip[4];

u16 ipv4_checksum(void *addr, i32 count) {
    u32 sum = 0;
    u16 *ptr = (u16 *)addr;

    while (count > 1) {
        sum += *ptr++;
        count -= 2;
    }

    if (count > 0) {
        sum += *((u8*)ptr);
    }
    while (sum >> 16) {
        sum = (sum & 0xFFFF) + (sum >> 16);
    }

    return ~sum;
}

void ipv4_receive_packet(void *packet) {
    u8 src_ip[4] = {0}, dest_ip[4] = {0};
    u16 expected_checksum, actual_checksum;
    bool my_packet;
    void *payload;
    ipv4_header_t *ipv4_header = (ipv4_header_t *)packet;

    u8 version = (ipv4_header->ihl_and_version >> 4) & 0xF;
    u8 ihl = ipv4_header->ihl_and_version & 0xF;
    u8 service = ipv4_header->service;
    u16 length = NTOHS(ipv4_header->length);
    u16 id = NTOHS(ipv4_header->id);
    u8 flags = (NTOHS(ipv4_header->flags) >> 13) & 0x7;
    u16 fragment_offset = NTOHS(ipv4_header->flags) & 0x1FFF;
    fragment_offset *= 8;
    u8 ttl = ipv4_header->ttl;
    u8 proto = ipv4_header->proto;
    u16 checksum = ipv4_header->checksum;
    payload = (void *)(((char *)packet) + (ihl * 4));
    debug("IPV4 PACKET\r\n");

    debug("version: %d\r\n", version);
    debug("ihl: %d\r\n", ihl);
    debug("service: %d\r\n", service);
    debug("length: %d\r\n", length);
    debug("id: %d\r\n", id);
    debug("flags: 0x%x, fragment offset - 0x%x\r\n", flags, fragment_offset);
    debug("ttl: %d\r\n", ttl);
    debug("proto: %d\r\n", proto);
    debug("checksum: 0x%x\r\n", checksum);

    src_ip[0] = ipv4_header->src_addr & 0xFF;
    src_ip[1] = (ipv4_header->src_addr & 0xFF00) >> 8;
    src_ip[2] = (ipv4_header->src_addr & 0xFF0000) >> 16;
    src_ip[3] = (ipv4_header->src_addr & 0xFF000000) >> 24;
    debug("Source IP address - %d.%d.%d.%d\r\n",
			src_ip[0], src_ip[1], src_ip[2], src_ip[3]
    );

    dest_ip[0] = ipv4_header->dest_addr & 0xFF;
    dest_ip[1] = (ipv4_header->dest_addr & 0xFF00) >> 8;
    dest_ip[2] = (ipv4_header->dest_addr & 0xFF0000) >> 16;
    dest_ip[3] = (ipv4_header->dest_addr & 0xFF000000) >> 24;
    debug("Destination IP address - %d.%d.%d.%d\r\n",
			dest_ip[0], dest_ip[1], dest_ip[2], dest_ip[3]
    );

    /* Validate checksum */
    expected_checksum = ipv4_header->checksum;
    ipv4_header->checksum = 0;
    actual_checksum = ipv4_checksum(ipv4_header, sizeof(ipv4_header_t));
    if (actual_checksum != expected_checksum) {
        debug("The checksum is invalid!\r\n");
	return;
    }

    /* Check that the packet belongs to me */
    my_packet = (my_ip[0] == dest_ip[0] &&
		    my_ip[1] == dest_ip[1] &&
		    my_ip[2] == dest_ip[2] &&
		    my_ip[3] == dest_ip[3]);
    if (!my_packet) {
        debug("This packet does not belong to me\r\n");
	return;
    }

    /* Support fragmentation */
    u16 plen = length - (ihl * 4);
    if ((flags & IPV4_FLAGS_MF) != 0 || fragment_offset != 0) {
        /* First fragment of group fragments */
        if ((flags & IPV4_FLAGS_MF) != 0 && fragment_offset == 0) {
            debug("First fragment of the upcoming group of fragments\r\n");
            create_first_fragment(id, payload, plen, fragment_offset);
	    return;
	} else if ((flags & IPV4_FLAGS_MF) != 0 && fragment_offset != 0) {
            debug("Nth fragment in the group of fragments\r\n");
	    add_fragment(id, payload, plen, fragment_offset);
	    return;
	} else if ((flags & IPV4_FLAGS_MF) == 0 && fragment_offset != 0) {
            debug("Last fragment in the group of fragments\r\n");
	    add_fragment(id, payload, plen, fragment_offset);
	    payload = compose_payload(id, &plen);
	    if (payload == NULL) {
                return;
	    }
	}
    }

    if (ipv4_header->proto == IPV4_PROTO_ICMP) {
        icmpv4_receive_packet(payload, plen, ipv4_header);
    } else if (ipv4_header->proto == IPV4_PROTO_TCP) {
        debug("received TCP fragment\r\n");
        /*tcp_receive_fragment(packet, packet_len);*/
    } else if (ipv4_header->proto == IPV4_PROTO_UDP) { 
        debug("received UDP datagram\r\n");
        udp_receive_datagram(packet, packet_len);
    } else {
        debug("Unknown IPv4 payload type...\r\n");
    }
}

void ipv4_send_packet(u8 *data, u32 len, u8 protocol, u32 dest_addr) {
    ipv4_header_t ipv4_header;
    u8 *packet, dest_ip[4] = {0}, dest_mac[6] = {0};
    u32 packet_len = sizeof(ipv4_header_t) + len;
    u32 src_addr = my_ip[0] | (my_ip[1] << 8) | (my_ip[2] << 16) | (my_ip[3] << 24);

    /* Check if the host is in my network */
    if ((dest_addr & netmask) == (src_addr & netmask)) {
        dest_ip[0] = dest_addr & 0xFF;
        dest_ip[1] = (dest_addr & 0xFF00) >> 8;
        dest_ip[2] = (dest_addr & 0xFF0000) >> 16;
        dest_ip[3] = (dest_addr & 0xFF000000) >> 24;
    } else {
        dest_ip[0] = 192;
        dest_ip[1] = 168;
        dest_ip[2] = 0;
        dest_ip[3] = 1;
    }

    u32 arp_sent = 3;
    while (!arp_lookup(dest_ip, dest_mac)) {
        if (arp_sent != 0) {
            --arp_sent;
            arp_send_packet(dest_ip);
        }
    }

    debug("IPV4 sending packet...\r\n");
    debug("MAC for %d.%d.%d.%d - %x:%x:%x:%x:%x:%x\r\n",
            dest_ip[0], dest_ip[1], dest_ip[2], dest_ip[3],
            dest_mac[0], dest_mac[1], dest_mac[2], dest_mac[3],
            dest_mac[4], dest_mac[5]
	);
    u16 x, offset = 0;
    while (len > 0) {
        if (len < (ETHER_MAX_PAYLOAD_LEN - sizeof(ipv4_header_t))) {
            x = len;
            len = 0;
        } else {
            len -= (ETHER_MAX_PAYLOAD_LEN - sizeof(ipv4_header_t));
            x = (ETHER_MAX_PAYLOAD_LEN - sizeof(ipv4_header_t));
        }
        ipv4_header.ihl_and_version = 0x45;
        ipv4_header.service = 0;
        ipv4_header.length = x + sizeof(ipv4_header_t);
        ipv4_header.length = HTONS(ipv4_header.length);
        ipv4_header.id = HTONS(ipv4_id);
        ipv4_header.flags = ((offset & 0x1FFF) / 8);
        if (x == (ETHER_MAX_PAYLOAD_LEN - sizeof(ipv4_header_t))) {
            ipv4_header.flags |= ((IPV4_FLAGS_MF) << 13);
        }
        ipv4_header.flags = HTONS(ipv4_header.flags);
        ipv4_header.ttl = 64;
        ipv4_header.proto = protocol;
        ipv4_header.checksum = 0;
        ipv4_header.src_addr = src_addr;
        ipv4_header.dest_addr = dest_addr;
        ipv4_header.checksum = ipv4_checksum(&ipv4_header, sizeof(ipv4_header_t));

        packet = malloc(x + sizeof(ipv4_header_t));
        memset((void *)packet, 0, x + sizeof(ipv4_header_t));
        memcpy(packet, &ipv4_header, sizeof(ipv4_header_t));
        memcpy(packet + sizeof(ipv4_header_t), data + offset, x);
        i32 ret = ethernet_send_frame(dest_mac, ETHER_TYPE_IP, packet, x + sizeof(ipv4_header_t));

        offset += x;
        free(packet);
    }
    ++ipv4_id;
}
