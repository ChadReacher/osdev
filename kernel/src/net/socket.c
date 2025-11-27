#include <net.h>
#include <socket.h>
#include <ipv4.h>
#include <vfs.h>
#include <panic.h>
#include <errno.h>

i32 socket_read(struct vfs_inode *inode, struct file *fp, i8 *buf, i32 count);
i32 socket_write(struct vfs_inode *inode, struct file *fp, i8 *buf, i32 count);
void socket_close(struct vfs_inode *inode, struct file *fp);

struct file_ops socket_file_ops = {
    NULL, // open
    socket_read,
    socket_write,
    NULL, // readdir
    socket_close,
};

struct vfs_inode *net_create_socket(i32 domain, i32 type, i32 protocol) {
    struct vfs_inode *inode = NULL;

    // domain is only supported for: AF_INET
    if (domain != AF_INET) {
        return NULL;
    }

    // type is only supported for: SOCK_STREAM (TCP), SOCK_DGRAM (UDP), SOCK_RAW (for ICMP?) 
    if (type != SOCK_DGRAM && type != SOCK_RAW && type != SOCK_STREAM) {
        return NULL;
    }
    
    // protocol is only supported for: 0 i.e. default protocol
    if (protocol != 0) {
        return NULL;
    }

    if (!(inode = get_empty_inode())) {
        return NULL;
    }
    inode->i_mode = S_IFSOCK;

    if (type == SOCK_RAW) {
        // create raw IP socket
        inode->u.i_socket.proto = protocol;
        net_create_ip_socket(inode);
    } else if (type == SOCK_STREAM) {
        // Create TCP socket
        assert(false && "todo");
    } else if (type == SOCK_DGRAM) {
        // Create UDP socket
        assert(false && "todo");
    }

    return inode;
}

i32 net_connect_socket(struct socket *socket, const struct sockaddr *addr, socklen_t addrlen) {
    if (!socket->ops->connect) {
        debug("[%s]: no connect() function\r\n", __func__);
        return -EINVAL;
    }
    if (socket->connected) {
        debug("[%s]: socket is already connected\r\n", __func__);
        return -EINVAL;
    }
    return socket->ops->connect(socket, addr, addrlen);
}

i32 net_bind_socket(struct socket *socket, const struct sockaddr *addr, socklen_t addrlen) {
    if (!socket->ops->bind) {
        debug("[%s]: no connect() function\r\n", __func__);
        return -EINVAL;
    }
    return socket->ops->bind(socket, addr, addrlen);
}

i32 net_send_socket(struct socket *socket, const void *buf, u32 len) {
    if (!socket->ops->send) {
        debug("[%s]: no connect() function\r\n", __func__);
        return -EINVAL;
    }
    return socket->ops->send(socket, buf, len);
}

i32 net_sendto_socket(struct socket *socket, const void *buf, u32 len, const struct sockaddr *addr, socklen_t addrlen) {
    if (!socket->ops->sendto) {
        debug("[%s]: no connect() function\r\n", __func__);
        return -EINVAL;
    }
    return socket->ops->sendto(socket, buf, len, addr, addrlen);
}

