#ifndef SOCKET_H
#define SOCKET_H

#include <types.h>

typedef i32 socklen_t;
typedef u32 sa_family_t;

struct sockaddr {
	// Socket address family
	sa_family_t sa_family;
	// Protocol address
	i8 sa_data[14];
};

#define SOCK_DGRAM		2
#define SOCK_RAW		4
#define SOCK_STREAM		8

#define AF_INET	2

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
