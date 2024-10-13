#include <ethernet.h>
#include <panic.h>
#include <string.h>
#include <heap.h>
#include <rtl8139.h>
#include <arp.h>
#include <ipv4.h>

extern u8 my_mac[6];

void ethernet_receive_frame(void *data, u32 data_len) {
    bool is_my_frame, is_broadcast;
    struct ethernet_header *ether_frame = (struct ethernet_header *)data;
    void *payload = (u8 *)data + sizeof(struct ethernet_header);

    debug("Receive ethernet frame\r\n");
    ether_frame->ethernet_type = NTOHS(ether_frame->ethernet_type);

    debug("Destination MAC address - %x:%x:%x:%x:%x:%x\r\n",
			ether_frame->dest_mac[0], ether_frame->dest_mac[1],
            ether_frame->dest_mac[2], ether_frame->dest_mac[3],
            ether_frame->dest_mac[4], ether_frame->dest_mac[5]
    );
    debug("Source MAC address - %x:%x:%x:%x:%x:%x\r\n",
			ether_frame->src_mac[0], ether_frame->src_mac[1],
            ether_frame->src_mac[2], ether_frame->src_mac[3],
            ether_frame->src_mac[4], ether_frame->src_mac[5]
    );

    is_my_frame = (ether_frame->dest_mac[0] == my_mac[0] &&
		    ether_frame->dest_mac[1] == my_mac[1] &&
		    ether_frame->dest_mac[2] == my_mac[2] &&
		    ether_frame->dest_mac[3] == my_mac[3] &&
		    ether_frame->dest_mac[4] == my_mac[4] &&
		    ether_frame->dest_mac[5] == my_mac[5]);
    is_broadcast = (ether_frame->dest_mac[0] == 0xFF && 
		    ether_frame->dest_mac[1] == 0xFF &&
		    ether_frame->dest_mac[2] == 0xFF &&
		    ether_frame->dest_mac[3] == 0xFF &&
		    ether_frame->dest_mac[4] == 0xFF &&
		    ether_frame->dest_mac[5] == 0xFF);

    if (!is_broadcast && !is_my_frame) {
        debug("Different MAC address, not my packet...\r\n");
        return;
    }

    if (ether_frame->ethernet_type == ETHER_TYPE_ARP) {
        arp_receive_packet(payload);
    } else if (ether_frame->ethernet_type == ETHER_TYPE_IP) {
        ipv4_receive_packet(payload);
    } else {
        debug("Unsupported ethernet type - 0x%x\r\n", 
				ether_frame->ethernet_type);
    }
}

i32 ethernet_send_frame(u8 *dest_mac, u16 ethernet_type, u8 *payload, u32 plen) {
    u8 frame[ETHER_MAX_FRAME_LEN];
    struct ethernet_header *hdr;
    u32 frame_len;

    if (!dest_mac || !payload || plen > ETHER_MAX_PAYLOAD_LEN) {
	    return -1;
    }
    memset(frame, 0, ETHER_MAX_FRAME_LEN);

    hdr = (struct ethernet_header *)frame;
    hdr->ethernet_type = HTONS(ethernet_type);
    memcpy(hdr->dest_mac, dest_mac, 6);
    memcpy(hdr->src_mac, my_mac, 6);

    memcpy(hdr + 1, payload, plen);
    frame_len = sizeof(struct ethernet_header) + 
	    (plen < ETHER_MIN_PAYLOAD_LEN ? ETHER_MIN_PAYLOAD_LEN : plen);

    rtl8139_transmit_data(frame, frame_len);

    return 0;
}
