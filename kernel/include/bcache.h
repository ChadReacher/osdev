#ifndef BCACHE_H
#define BCACHE_H

#include "types.h"

#define BLOCK_SIZE 1024

#define NBUF 5

#define B_INVALID   0x1
#define B_USED      0x2
#define B_DIRTY     0x4

struct buffer {
    i32 refcnt;
    i32 flags;
    u16 dev;
    u32 block;
    i8 *data;
    struct buffer *prev;
    struct buffer *next;
};

void bcache_init(void);
struct buffer *bread(u16 dev, u32 block);
void bwrite(struct buffer *buf);
void brelse(struct buffer *buf);
void sync_buffers(void);

#endif
