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

    for (b = head.next; b != &head; b = b->next) {
        if (b->dev == dev && b->block == block) {
            ++b->refcnt;
            return b;
        }
    }

    for (b = head.prev; b != &head; b = b->prev) {
        if (b->refcnt > 0) {
            continue;
        } else if ((b->flags & B_DIRTY) == B_DIRTY) {
            blk_dev_write(b);
        }
        b->refcnt = 1;
        b->flags = B_INVALID;
        b->dev = dev;
        b->block = block;
        return b;
    }

    panic("[bcache]: no buffers\r\n");
    return NULL;
}

struct buffer *bread(u16 dev, u32 block) {
    struct buffer *b = bget(dev, block);

    if ((b->flags & B_INVALID) == B_INVALID) {
        blk_dev_read(b);
        b->flags = B_USED;
    }
    return b;
}

void bwrite(struct buffer *buf) {
    assert(buf != NULL);
    assert((buf->flags & B_INVALID) != B_INVALID);

    buf->flags |= B_DIRTY;
}

void brelse(struct buffer *buf) {
    if (!buf) {
        return;
    }
    assert(buf->refcnt > 0);

    --buf->refcnt;
    if (buf->refcnt) {
        return;
    }
    buf->next->prev = buf->prev;
    buf->prev->next = buf->next;
    buf->next = head.next;
    buf->prev = &head;
    head.next->prev = buf;
    head.next = buf;
}

