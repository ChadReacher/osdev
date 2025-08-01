#ifndef SOCKET_H
#define SOCKET_H

#include <types.h>

typedef i32 socklen_t;
typedef u32 sa_family_t;

struct sockaddr {
	// Socket address family
	sa_family_t sa_family;
	// Socket address (variable-length data)
	i8 sa_data[];
};

#define SOCK_DGRAM		2
#define SOCK_RAW		4
#define SOCK_STREAM		8

#define AF_INET	2

#endif
