#include <arp.h>
#include <panic.h>
#include <ethernet.h>
#include <string.h>
#include <heap.h>

#define MAX_ARP_ENTRIES 32

u8 my_ip[4] = { 192, 168, 0, 116 };

extern u8 my_ip[4];
extern u8 my_mac[6];

struct arp_entry {
    u32 ip_addr;
    u8 mac_addr[6];
};

static u32 arp_idx;
static struct arp_entry arp_table[MAX_ARP_ENTRIES];

void arp_receive_packet(void *packet) {
    struct arp_packet *arp_packet = (struct arp_packet *)packet;

    debug("Received ARP packet...\r\n");
    arp_packet->hardware_type = NTOHS(arp_packet->hardware_type);
    arp_packet->protocol_type = NTOHS(arp_packet->protocol_type);
    arp_packet->operation = NTOHS(arp_packet->operation);

    debug("hardware type - 0%x\r\n" "protocol_type - 0x%x\r\n"
          "hardware_addr_len - 0x%x\r\n" "protocol_addr_len - 0x%x\r\n"
          "operation - 0x%x\r\n",
          arp_packet->hardware_type, arp_packet->protocol_type,
          arp_packet->hardware_addr_len, arp_packet->protocol_addr_len,
          arp_packet->operation
         );   

    debug("Source MAC address - %x:%x:%x:%x:%x:%x, ",
			arp_packet->src_mac[0], arp_packet->src_mac[1],
            arp_packet->src_mac[2], arp_packet->src_mac[3],
            arp_packet->src_mac[4], arp_packet->src_mac[5]
    );

    debug("Source IP address - %d.%d.%d.%d\r\n",
			arp_packet->src_ip[0], arp_packet->src_ip[1],
            arp_packet->src_ip[2], arp_packet->src_ip[3]
    );

    debug("Destination MAC address - %x:%x:%x:%x:%x:%x, ",
			arp_packet->dest_mac[0], arp_packet->dest_mac[1],
            arp_packet->dest_mac[2], arp_packet->dest_mac[3],
            arp_packet->dest_mac[4], arp_packet->dest_mac[5]
    );

    debug("Destination IP address - %d.%d.%d.%d\r\n",
			arp_packet->dest_ip[0], arp_packet->dest_ip[1],
            arp_packet->dest_ip[2], arp_packet->dest_ip[3]
    );

    bool is_my_packet = (arp_packet->dest_ip[0] == my_ip[0] &&
            arp_packet->dest_ip[1] == my_ip[1] &&
            arp_packet->dest_ip[2] == my_ip[2] &&
            arp_packet->dest_ip[3] == my_ip[3]);
    if (!is_my_packet) {
	    debug("[ARP]: Received a packet not for me\r\n"); 
	    return;
    }
    if (arp_packet->operation == ARP_REQUEST) {
        debug("This ARP packet is request\r\n");
        struct arp_packet reply;

        reply.hardware_type = HTONS(arp_packet->hardware_type);
        reply.protocol_type = HTONS(arp_packet->protocol_type);
        reply.hardware_addr_len = 6;
        reply.protocol_addr_len = 4;
        reply.operation = HTONS(ARP_REPLY);
        memcpy(reply.src_mac, my_mac, 6);
        memcpy(reply.src_ip, my_ip, 4);
        memcpy(reply.dest_mac, arp_packet->src_mac, 6);
        memcpy(reply.dest_ip, arp_packet->src_ip, 4);
        i32 err = ethernet_send_frame(arp_packet->src_mac, ETHER_TYPE_ARP, (u8 *)&reply,
		       	sizeof(struct arp_packet));
    } else if (arp_packet->operation == ARP_REPLY) {
        debug("This ARP packet is reply\r\n");
	memcpy(&arp_table[arp_idx].ip_addr, arp_packet->src_ip, 4);
	memcpy(&arp_table[arp_idx].mac_addr, arp_packet->src_mac, 6);
	arp_idx = (arp_idx + 1) % MAX_ARP_ENTRIES;
    }
}

void arp_send_packet(u8 *dst_ip) {
    struct arp_packet arp_packet;
    u32 packet_len = sizeof(struct arp_packet);
    u8 broadcast[6] = { 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF };

    debug("Sending ARP packet...\r\n");

    arp_packet.hardware_type = HTONS(1);
    arp_packet.protocol_type = HTONS(ETHER_TYPE_IP);
    arp_packet.hardware_addr_len = 6;
    arp_packet.protocol_addr_len = 4;
    arp_packet.operation = HTONS(ARP_REQUEST);
    memcpy(arp_packet.src_mac, my_mac, 6);
    memcpy(arp_packet.src_ip, my_ip, 4);
    memcpy(arp_packet.dest_mac, broadcast, 6);
    memcpy(arp_packet.dest_ip, dst_ip, 4);

    i32 err = ethernet_send_frame(broadcast, ETHER_TYPE_ARP, &arp_packet, packet_len);
}

/* Tries to find an arp entry.
 * Return true if found
 * false if didn't find
 */
u32 arp_lookup(u8 *dest_ip, u8 *dest_mac) {
    u32 ip_entry = *((u32 *)(dest_ip));
    u32 i;
    for (i = 0; i < MAX_ARP_ENTRIES; ++i) {
        if (arp_table[i].ip_addr == ip_entry) {
            memcpy(dest_mac, &arp_table[i].mac_addr, 6);
            return 1;
        }
    }
    return 0;
}
