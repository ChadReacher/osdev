#include <udp.h>
#include <panic.h>
#include <ipv4.h>
#include <ethernet.h>

extern u8 my_ip[4];

void udp_receive_datagram(void *datagram, u32 datagram_len) {
	struct udp_header *hdr;
	hdr = (struct udp_header *)datagram;
	debug("Source port - %d, destination port - %d, length - %d, checksum - 0x%x\r\n",
		hdr->src_port, hdr->dest_port, hdr->length, hdr->checksum
	);
}

void udp_transmit_datagram(u32 src_port, u32 dest_ip, u32 dest_port) {
	debug("UDP sending datagram...\r\n");
	u8 *packet, *d;
	u32 myip, len = sizeof(struct udp_header) + 5;
	struct udp_header hdr;
	/* checksum is optional for IPv4 */
	hdr.checksum = 0;
	hdr.src_port = HTONS(src_port);
	hdr.dest_port = HTONS(dest_port);
	hdr.length = sizeof(struct udp_header) + 5;
	hdr.length = HTONS(hdr.length);


	d = malloc(5);
	d[0] = 'X'; d[1] = 'Y'; d[2] = 'Z'; d[3] = '5'; d[4] = '0';
	packet = malloc(len);
	memcpy(packet, &hdr, sizeof(struct udp_header));
	memcpy(packet + sizeof(struct udp_header), d, 5);
	
	myip = my_ip[0] | (my_ip[1] << 8) | (my_ip[2] << 16) | (my_ip[3] << 24);
	ipv4_send_packet(packet, len, 0, IPV4_PROTO_UDP, myip, dest_ip);
	free(d);
	free(packet);
}

