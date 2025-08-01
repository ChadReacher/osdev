#ifndef SOCKET_H
#define SOCKET_H

#include <sys/types.h>

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

i32     accept(i32 sockfd, struct sockaddr *addr, socklen_t *addrlen);
i32     bind(i32 sockfd, const struct sockaddr *addr, socklen_t addrlen);
i32     connect(i32 sockfd, const struct sockaddr *addr, socklen_t addrlen);
i32     listen(i32 sockfd, i32 backlog);
i32		recv(i32 sockfd, void *buf, u32 len, i32 flags);
i32		recvfrom(i32 sockfd, void *buf, u32 len, i32 flags, struct sockaddr *src_addr, 
			socklen_t *addrlen);
i32		send(i32 sockfd, const void *buf, u32 len, i32 flags);
i32		sendto(i32 sockfd, const void *buf, u32 len, i32 flags, const struct sockaddr *dest_addr,
			socklen_t addrlen);
i32     shutdown(i32 sockfd, i32 how);
i32     socket(i32 domain, i32 type, i32 protocol);

//int     sockatmark(int);
//int     socketpair(int, int, int, int[2]);
//int     getpeername(int, struct sockaddr *restrict, socklen_t *restrict);
//int     getsockname(int, struct sockaddr *restrict, socklen_t *restrict);
//int     getsockopt(int, int, int, void *restrict, socklen_t *restrict);
//int     setsockopt(int, int, int, const void *, socklen_t);

#endif
