#ifndef NET_H
#define NET_H

#include <types.h>
#include <socket.h>

struct socket;

struct socket_ops {
    i32 (*connect)(struct socket *socket, const struct sockaddr *addr, socklen_t addrlen);
    i32 (*bind)(struct socket *socket, const struct sockaddr *addr, socklen_t addrlen);
    void (*listen)(void);
    i32 (*send)(struct socket *socket, const void *buf, u32 len, i32 flags);
    i32 (*sendto)(struct socket *socket, const void *buf, u32 len, i32 flags, const struct sockaddr *addr, socklen_t addrlen);
    void (*recv)(void);
    void (*recvfrom)(void);
};

struct socket {
    i32 proto;
    bool connected;
    bool bounded;
    struct sockaddr daddr;
    struct sockaddr saddr;
    struct socket_ops *ops;
};

struct vfs_inode *net_create_socket(i32 domain, i32 type, i32 protocol);
i32 net_connect_socket(struct socket *socket, const struct sockaddr *addr, socklen_t addrlen);
i32 net_bind_socket(struct socket *socket, const struct sockaddr *addr, socklen_t addrlen);
i32 net_send_socket(struct socket *socket, const void *buf, u32 len, i32 flags);
i32 net_sendto_socket(struct socket *socket, const void *buf, u32 len, i32 flags, const struct sockaddr *addr, socklen_t addrlen);

#endif
