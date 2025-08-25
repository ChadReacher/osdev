#include <ext2.h>
#include <timer.h>
#include <bcache.h>
#include <vfs.h>
#include <errno.h>

static void trunc_direct(struct vfs_inode *inode, u32 block) {
    for (u32 i = block; i < EXT2_DIRECT_BLOCKS; ++i) {
        if (!inode->u.i_ext2.i_block[i]) {
            continue;
        }
        ext2_free_block(inode->i_dev, inode->u.i_ext2.i_block[i]);
        inode->u.i_ext2.i_block[i] = 0;
        inode->i_blocks -= 2;
    }
}

static void trunc_indirect(struct vfs_inode *inode, u32 block) {
    if (!inode->u.i_ext2.i_block[12]) {
        return;
    }
    struct buffer *buf = bread(inode->i_dev, inode->u.i_ext2.i_block[12]);
    u32 *blocks = (u32 *)buf->data;
    for (u32 i = block; i < EXT2_POINTERS_PER_BLOCK; ++i) {
        if (!blocks[i]) {
            continue;
        }
        ext2_free_block(inode->i_dev, blocks[i]);
        blocks[i] = 0;
        inode->i_blocks -= 2;
    }
    bwrite(buf);
    brelse(buf);

    ext2_free_block(inode->i_dev, inode->u.i_ext2.i_block[12]);
    inode->u.i_ext2.i_block[12] = 0;
    inode->i_blocks -= 2;
}

static void trunc_doubly_indirect(struct vfs_inode *inode, u32 block) {
    if (!inode->u.i_ext2.i_block[13]) {
        return;
    }
    struct buffer *buf = bread(inode->i_dev, inode->u.i_ext2.i_block[13]);
    u32 *indirect = (u32 *)buf->data;
    for (u32 i = 0; i < EXT2_POINTERS_PER_BLOCK; ++i) {
        if (!indirect[i]) {
            continue;
        }
        struct buffer *buf2 = bread(inode->i_dev, indirect[i]);
        u32 *blocks = (u32 *)buf2->data;
        for (u32 j = block; j < EXT2_POINTERS_PER_BLOCK; ++j) {
            if (!blocks[j]) {
                continue;
            }
            ext2_free_block(inode->i_dev, blocks[j]);
            blocks[j] = 0;
            inode->i_blocks -= 2;
        }
        bwrite(buf2);
        brelse(buf2);

        ext2_free_block(inode->i_dev, indirect[i]);
        indirect[i] = 0;
        inode->i_blocks -= 2;

    }
    bwrite(buf);
    brelse(buf);

    ext2_free_block(inode->i_dev, inode->u.i_ext2.i_block[13]);
    inode->u.i_ext2.i_block[13] = 0;
    inode->i_blocks -= 2;
}

static void trunc_triply_indirect(struct vfs_inode *inode, u32 block) {
    if (!inode->u.i_ext2.i_block[14]) {
        return;
    }
    struct buffer *buf = bread(inode->i_dev, inode->u.i_ext2.i_block[14]);
    u32 *dindirect = (u32 *)buf->data;
    for (u32 i = 0; i < EXT2_POINTERS_PER_BLOCK; ++i) {
        if (!dindirect[i]) {
            continue;
        }
        struct buffer *buf2 = bread(inode->i_dev, dindirect[i]);
        u32 *indirect = (u32 *)buf2->data;
        for (u32 j = 0; j < EXT2_POINTERS_PER_BLOCK; ++j) {
            if (!indirect[j]) {
                continue;
            }
            struct buffer *buf3 = bread(inode->i_dev, indirect[j]);
            u32 *blocks = (u32 *)buf3->data;
            for (u32 k = block; k < EXT2_POINTERS_PER_BLOCK; ++k) {
                if (!blocks[k]) {
                    continue;
                }
                ext2_free_block(inode->i_dev, blocks[k]);
                blocks[k] = 0;
                inode->i_blocks -= 2;
            }
            bwrite(buf3);
            brelse(buf3);

            ext2_free_block(inode->i_dev, indirect[j]);
            indirect[j] = 0;
            inode->i_blocks -= 2;
        }
        bwrite(buf2);
        brelse(buf2);

        ext2_free_block(inode->i_dev, dindirect[i]);
        dindirect[i] = 0;
        inode->i_blocks -= 2;
    }
    bwrite(buf);
    brelse(buf);

    ext2_free_block(inode->i_dev, inode->u.i_ext2.i_block[14]);
    inode->u.i_ext2.i_block[14] = 0;
    inode->i_blocks -= 2;
}

// For now, we keep the invariant the given `length` is always less then the
// actual file size `inode->i_size`
i32 ext2_truncate(struct vfs_inode *inode, u32 length) {
    if (length > inode->i_size) {
        // NB: Current implementation does not support extending a file beyond
        // its current size.
        vfs_iput(inode);
        return -EPERM;
    }

    u32 block = (length + inode->i_sb->s_block_size - 1) / inode->i_sb->s_block_size;

    if (block < EXT2_DIRECT_BLOCKS) {
        trunc_direct(inode, block);
        block = 0;
    }

    if (block) {
        block -= EXT2_DIRECT_BLOCKS;
    }
    if (block < EXT2_DIRECT_BLOCKS + EXT2_POINTERS_PER_BLOCK * 1) {
        trunc_indirect(inode, block);
        block = 0;
    }

    if (block) {
        block -= EXT2_POINTERS_PER_BLOCK;
    }
    if (block < EXT2_DIRECT_BLOCKS + EXT2_POINTERS_PER_BLOCK * 2) {
        trunc_doubly_indirect(inode, block);
        block = 0;
    }

    if (block) {
        block -= EXT2_POINTERS_PER_BLOCK;
    }
    if (block < EXT2_DIRECT_BLOCKS + EXT2_POINTERS_PER_BLOCK * 3) {
        trunc_triply_indirect(inode, block);
        block = 0;
    }

    inode->i_size = length;
    inode->i_mtime = inode->i_ctime = get_current_time();
    inode->i_dirt = 1;
    return 0;
}

