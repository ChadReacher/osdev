#include "lock.h"
#include "process.h"
#include <bcache.h>
#include <blk_dev.h>
#include <panic.h>
#include <heap.h>
#include <string.h>

struct buffer bufs[NBUF];
struct buffer head;

void bcache_init(void) {
    head.prev = &head;
    head.next = &head;
    for (i32 i = 0; i < NBUF; ++i) {
        struct buffer *b = &bufs[i];
        b->next = head.next;
        b->prev = &head;
        head.next->prev = b;
        head.next = b;
        b->data = malloc(BLOCK_SIZE);
        if (b->data == NULL) {
            panic("failed to allocate enough memory for block cache page");
        }
        memset(b->data, 0, BLOCK_SIZE);
    }
    debug("Buffer cache (capacity = %d) has been initialized\r\n", NBUF);
}

void sync_buffers(void) {
    debug("[bcache]: syncing buffers\r\n");
    for (i32 i = 0; i < NBUF; ++i) {
        struct buffer *b = &bufs[i];
        if ((b->flags & B_DIRTY) == B_DIRTY) {
            blk_dev_write(b);
        }
    }
}

static struct buffer *bget(u16 dev, u32 block) {
    struct buffer *b = NULL;

    again:
    for (b = head.next; b != &head; b = b->next) {
        if (b->dev == dev && b->block == block) {
            if ((b->flags & B_LOCKED) == B_LOCKED) {
                chan_sleep(b);
                goto again;
            }
            b->flags |= B_LOCKED;
            ++b->refcnt;
            return b;
        }
    }

    for (b = head.prev; b != &head; b = b->prev) {
        if ((b->flags & B_LOCKED) || b->refcnt > 0) {
            continue;
        }

        b->flags |= B_LOCKED;

        if ((b->flags & B_DIRTY) == B_DIRTY) {
            blk_dev_write(b);
            b->flags &= ~B_DIRTY;
            b->flags &= ~B_LOCKED;
            goto again;
        }
        b->refcnt = 1;
        b->flags = B_LOCKED | B_INVALID;
        b->dev = dev;
        b->block = block;
        return b;
    }

    debug("[bcache]: no buffers\r\n");
    chan_sleep((void*)bget);
    goto again;
    return NULL;
}

struct buffer *bread(u16 dev, u32 block) {
    struct buffer *b = bget(dev, block);
    assert(b != NULL);

    assert(b->flags & B_LOCKED);
    if ((b->flags & B_INVALID) == B_INVALID) {
        blk_dev_read(b);
        b->flags &= ~B_INVALID;
        b->flags |= B_USED;
    }
    assert(b->dev == dev);
    assert(b->block == block);
    return b;
}

void bwrite(struct buffer *buf) {
    assert(buf != NULL);
    assert((buf->flags & B_INVALID) != B_INVALID);
    assert(buf->flags & B_LOCKED);

    buf->flags |= B_DIRTY;
}

void brelse(struct buffer *buf) {
    if (!buf) {
        return;
    }
    assert(buf->refcnt > 0);

    gl_lock();


    --buf->refcnt;
    if (buf->refcnt == 0) {
        buf->next->prev = buf->prev;
        buf->prev->next = buf->next;
        buf->next = head.next;
        buf->prev = &head;
        head.next->prev = buf;
        head.next = buf;
    }

    buf->flags &= ~B_LOCKED;
    chan_wakeup(buf);
    chan_wakeup((void *)bget);

    gl_unlock();
}
