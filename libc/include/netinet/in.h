#ifndef NETINET_IN_H
#define NETINET_IN_H

#include <sys/types.h>
#include <sys/socket.h>

typedef u16 in_port_t;
typedef u32 in_addr_t;

struct in_addr {
	in_addr_t s_addr;
};

struct sockaddr_in {
	sa_family_t sin_family;		// AF_INET
	in_port_t sin_port;			// Port number
	struct in_addr sin_addr;	// IP address
};

#define INADDR_ANY 0x00000000

#endif
