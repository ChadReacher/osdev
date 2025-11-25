#ifndef PIPE_H
#define PIPE_H

#include "types.h"

#define PIPE_HEAD(inode) (inode->u.i_pipe.i_head)
#define PIPE_TAIL(inode) (inode->u.i_pipe.i_tail)
#define PIPE_SIZE(inode) ((PIPE_HEAD(inode)-PIPE_TAIL(inode))&(4096-1))
#define PIPE_FULL(inode) (PIPE_SIZE(inode) == (4096-1))
#define PIPE_EMPTY(inode) (PIPE_HEAD(inode) == PIPE_TAIL(inode))


struct file_ops;
struct vfs_inode;

struct pipe_inode {
	i8 *i_buf;
	u32 i_head;
    u32 i_tail;
};

extern struct file_ops pipe_read_ops;
extern struct file_ops pipe_write_ops;

struct vfs_inode *pipe_get_inode(void);

#endif
