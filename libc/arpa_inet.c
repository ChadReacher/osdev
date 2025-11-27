#include <arpa/inet.h>

#define NTOHS(n) ((((n) & 0xFF) << 8) | (((n) & 0xFF00) >> 8))
#define HTONS(n) NTOHS(n)

// 0xAABBCCDD -> 0xDDCCBBAA
u32 htonl(u32 hostlong) {
	return (((hostlong) & 0xFF) << 24) | 
		(((hostlong) & 0xFF00) << 8) |
		(((hostlong) & 0xFF0000) >> 8) |
		(((hostlong) & 0xFF000000) >> 24);
}

// 0xAABB -> 0xBBAA
u16 htons(u16 hostshort) {
	return (((hostshort) & 0xFF) << 8) | (((hostshort) & 0xFF00) >> 8);
}

u32 ntohl(u32 netlong) {
	return htonl(netlong);
}

u16 ntohs(u16 netshort) {
	return htons(netshort);
}

in_addr_t inet_addr(const i8 *cp) {
    (void)cp;
    // TODO: implement
    return INADDR_ANY;
}
