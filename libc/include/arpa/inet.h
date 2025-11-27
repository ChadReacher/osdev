#ifndef ARPA_INET_H
#define ARPA_INET_H

#include <sys/types.h>
#include <netinet/in.h>

u32 htonl(u32 hostlong);
u16 htons(u16 hostshort);
u32 ntohl(u32 netlong);
u16 ntohs(u16 netshort);

in_addr_t inet_addr(const i8 *cp);

#endif
