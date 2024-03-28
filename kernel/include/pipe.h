#ifndef PIPE_H
#define PIPE_H

#include "types.h"
#include "ext2.h" 

#define PIPE_HEAD(inode) (inode->i_block[1])
#define PIPE_TAIL(inode) (inode->i_block[2])
#define PIPE_SIZE(inode) ((PIPE_HEAD(inode)-PIPE_TAIL(inode))&(4096-1))
#define PIPE_FULL(inode) (PIPE_SIZE(inode) == (4096-1))
#define PIPE_EMPTY(inode) (PIPE_HEAD(inode) == PIPE_TAIL(inode))

i32 read_pipe(struct ext2_inode *inode, i8 *buf, u32 count);
i32 write_pipe(struct ext2_inode *inode, i8 *buf, u32 count);

#endif
