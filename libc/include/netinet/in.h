#ifndef NETINET_IN_H
#define NETINET_IN_H

#include <sys/types.h>
#include <sys/socket.h>

typedef u16 in_port_t;
typedef u32 in_addr_t;

struct in_addr {
	in_addr_t s_addr;
};

// IPv4 only
struct sockaddr_in {
	sa_family_t sin_family;		// AF_INET
	in_port_t sin_port;			// Port number
	struct in_addr sin_addr;	// IP address
    u8 sin_zero[8]; // Same size as struct sockaddr
};

// Supported IP protocols
#define IPPROTO_IP      0
#define IPPROTO_ICMP    1

#define INADDR_ANY ((in_addr_t) 0x00000000)

#endif
