#include "errno.h"
#include "unistd.h"
#include "sys/socket.h"

syscall3(i32, accept, i32, sockfd, struct sockaddr *, addr, socklen_t *, addrlen)
syscall3(i32, bind, i32, sockfd, const struct sockaddr *, addr, socklen_t, addrlen)
syscall3(i32, connect, i32, sockfd, const struct sockaddr *, addr, socklen_t, addrlen)
syscall2(i32, listen, i32, sockfd, i32, backlog)
syscall4(i32, recv, i32, sockfd, void *, buf, u32, len, i32, flags)

i32 recvfrom(i32 sockfd, void *buf, u32 len, i32 flags, struct sockaddr *src_addr, 
			socklen_t *addrlen) {
	i32 ret;
    (void)addrlen;

	__asm__ volatile ("int $0x80" 
			: "=a"(ret) 
			: "a"(__NR_recvfrom), "b"(sockfd), "c"(buf), "d"(len), "S"(flags), "D"(src_addr));

	return ret;
}

syscall4(i32, send, i32, sockfd, const void *, buf, u32, len, i32, flags)

i32 sendto(i32 sockfd, const void *buf, u32 len, i32 flags, const struct sockaddr *dest_addr,
			socklen_t addrlen) {
	i32 ret;

	__asm__ __volatile__ ("int $0x80" 
			: "=a"(ret) 
			: "a"(__NR_sendto), "b"(sockfd), "c"(buf), "d"(len), "S"(flags), "D"(dest_addr));

	return ret;
}

syscall2(i32, shutdown, i32, sockfd, i32, how)

syscall3(i32, socket, i32, domain, i32, type, i32, protocol)
