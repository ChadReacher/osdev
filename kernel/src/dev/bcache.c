#include <bcache.h>
#include <blk_dev.h>
#include <panic.h>
#include <heap.h>
#include <string.h>

struct buffer *bufs[NBUF];
struct buffer head;

void bcache_init(void) {
        for (i32 i = 0; i < NBUF; ++i) {
                bufs[i] = malloc(sizeof(struct buffer));
                memset(bufs[i], 0, sizeof(struct buffer));
        }

        head.prev = &head;
        head.next = &head;
        for (i32 i = 0; i < NBUF; ++i) {
                struct buffer *b = bufs[i];
		b->next = head.next;
		b->prev = &head;
		head.next->prev = b;
		head.next = b;
		b->data = malloc(BLOCK_SIZE);
                memset(b->data, 0, BLOCK_SIZE);
        }
        debug("Buffer cache (cap = %d) has been initialized\r\n", NBUF);
}

void sync_buffers(void) {
        debug("[bcache]: syncing buffers\r\n");
        for (i32 i = 0; i < NBUF; ++i) {
                struct buffer *b = bufs[i];
                if ((b->flags & B_DIRTY) == B_DIRTY) {
                        blk_dev_write(b);
                }
        }
}

static struct buffer *bget(u16 dev, u32 block) {
	struct buffer *b;

	for (b = head.next; b != &head; b = b->next) {
		if (b->dev == dev && b->block == block) {
                        ++b->refcnt;
			return b;
		}
	}
	
	for (b = head.prev; b != &head; b = b->prev) {
		if ((b->flags & B_DIRTY) == B_DIRTY) {
			blk_dev_write(b);
		} else if (b->refcnt > 0) {
                        continue;
                }
		b->dev = dev;
		b->block = block;
		b->flags = B_INVALID;
                b->refcnt = 1;
		return b;
	}
	panic("[bcache]: no buffers\r\n");
	return NULL;
}

struct buffer *bread(u16 dev, u32 block) {
	struct buffer *b;

	b = bget(dev, block);
	if ((b->flags & B_INVALID) == B_INVALID) {
		blk_dev_read(b);
		b->flags = B_USED;
	}
	return b;
}

void bwrite(struct buffer *buf) {
	if (!buf) {
		return;
	}
	buf->flags |= B_DIRTY;
}

void brelse(struct buffer *buf) {
	if (!buf) {
		return;
	}
        --buf->refcnt;
        if (buf->refcnt) {
                return;
        }
        buf->refcnt = 0;
	buf->next->prev = buf->prev;
	buf->prev->next = buf->next;
	buf->next = head.next;
	buf->prev = &head;
	head.next->prev = buf;
	head.next = buf;
}


