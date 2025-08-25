#include <ext2.h>
#include <blk_dev.h>
#include <timer.h>
#include <vfs.h>
#include <panic.h>
#include <bcache.h>

void ext2_free_block(u16 dev, u32 block) {
    struct ext2_blk_grp_desc bgd;
    u32 bitmap_block;
    u32 sub_bitmap_idx, idx, mask;
    struct vfs_superblock *vsb = get_vfs_super(dev);

    u32 group = block / vsb->u.ext2_sb.s_blocks_per_group;
    if (read_group_desc(&bgd, group, vsb) != 0) {
        debug("Failed to read group descriptor #%d\r\n", group);
        return;
    }
    bitmap_block = bgd.bg_block_bitmap;

    struct buffer *buf = bread(dev, bitmap_block);
    u32 *bdata = (u32 *)buf->data;

    sub_bitmap_idx = (block - vsb->u.ext2_sb.s_blocks_per_group * group) / 32;
    idx = (block - vsb->u.ext2_sb.s_blocks_per_group * group) % 32;
    --idx;
    
    mask = ~(1 << idx);
    bdata[sub_bitmap_idx] = bdata[sub_bitmap_idx] & mask;
    
    bwrite(buf);
    brelse(buf);

    ++bgd.bg_free_blocks_count;
    ++vsb->u.ext2_sb.s_free_blocks_count;
    if (write_group_desc(&bgd, group, vsb) != 0) {
        debug("Failed to write group descriptor #%d\r\n", group);
        return;
    }
}

u32 ext2_alloc_block(u16 dev) {
    struct vfs_superblock *vsb = get_vfs_super(dev);

    u32 s_total_groups = vsb->u.ext2_sb.s_blocks_count / vsb->u.ext2_sb.s_blocks_per_group;
    for (u32 i = 0; i < s_total_groups; ++i) {
        struct ext2_blk_grp_desc bgd;

        if (read_group_desc(&bgd, i, vsb) != 0) {
            debug("Failed to read group descriptor #%d\r\n", i);
            continue;
        }
        if (!bgd.bg_free_blocks_count) {
            continue;
        }
        u32 bitmap_block = bgd.bg_block_bitmap;
        struct buffer *buf = bread(dev, bitmap_block);
        u32 *bdata = (u32 *)buf->data;
        for (u32 j = 0; j < vsb->s_block_size / 4; ++j) {
            u32 sub_bitmap = bdata[j];
            if (sub_bitmap == 0xFFFFFFFF) {
                continue;
            }
            for (u32 k = 0; k < 32; ++k) {
                u32 is_free = (sub_bitmap & (1 << k)) == 0;
                if (!is_free) {
                    continue;
                }
                u32 mask = 1 << k;
                bdata[j] = bdata[j] | mask;
                bwrite(buf);

                --bgd.bg_free_blocks_count;
                if (write_group_desc(&bgd, i, vsb) != 0) {
                    debug("Failed to write group descriptor #%d\r\n", i);
                }

                brelse(buf);
                return i * vsb->u.ext2_sb.s_blocks_per_group + j * 32 + k + 1;
            }
        }
        brelse(buf);
    }
    return 0;
}

void ext2_free_inode(struct vfs_inode *inode) {
    struct ext2_blk_grp_desc bgd; 
    u32 bitmap_inode;
    u32 sub_bitmap_idx, idx, mask;

    struct vfs_superblock *vsb = inode->i_sb;
    u32 group = inode->i_num / vsb->u.ext2_sb.s_inodes_per_group;
    if (read_group_desc(&bgd, group, vsb) != 0) {
        debug("Failed to read group descriptor #%d\r\n", group);
        return;
    }
    bitmap_inode = bgd.bg_inode_bitmap;

    struct buffer *buf = bread(inode->i_dev, bitmap_inode);
    u32 *bdata = (u32 *)buf->data;

    sub_bitmap_idx = (inode->i_num - vsb->u.ext2_sb.s_inodes_per_group * group) / 32; 
    idx = (inode->i_num - vsb->u.ext2_sb.s_inodes_per_group * group) % 32;
    --idx;
    
    mask = ~(1 << idx);
    bdata[sub_bitmap_idx] = bdata[sub_bitmap_idx] & mask;
    
    bwrite(buf);
    brelse(buf);

    ++bgd.bg_free_inodes_count;
    ++vsb->u.ext2_sb.s_free_inodes_count;
    if (write_group_desc(&bgd, group, vsb) != 0) {
        debug("Failed to read group descriptor #%d\r\n", group);
        return;
    }
}

struct vfs_inode *ext2_alloc_inode(u16 dev) {
    struct vfs_superblock *vsb = get_vfs_super(dev);
    struct vfs_inode *inode = get_empty_inode();

    u32 s_total_groups = vsb->u.ext2_sb.s_blocks_count / vsb->u.ext2_sb.s_blocks_per_group;
    for (u32 i = 0; i < s_total_groups; ++i) {
        struct ext2_blk_grp_desc bgd; 

        if (read_group_desc(&bgd, i, vsb) != 0) {
            debug("Failed to read group descriptor #%d\r\n", i);
            continue;
        }
        if (!bgd.bg_free_inodes_count) {
            continue;
        }
        u32 bitmap_block = bgd.bg_inode_bitmap;
        struct buffer *buf = bread(dev, bitmap_block);
        u32 *bdata = (u32 *)buf->data;
        for (u32 j = 0; j < vsb->s_block_size / 4; ++j) {
            u32 sub_bitmap = bdata[j];
            if (sub_bitmap == 0xFFFFFFFF) {
                continue;
            }
            for (u32 k = 0; k < 32; ++k) {
                u32 is_free = !(sub_bitmap & (1 << k));
                if (!is_free) {
                    continue;
                }
                u32 mask = 1 << k;
                bdata[j] = bdata[j] | mask;
                bwrite(buf);

                --bgd.bg_free_inodes_count;
                if (write_group_desc(&bgd, i, vsb) != 0) {
                    debug("Failed to write group descriptor #%d\r\n", i);
                }
                inode->i_num = i * vsb->u.ext2_sb.s_inodes_per_group + j * 32 + k + 1;
                inode->i_count = 1;
                inode->i_links_count = 1;
                inode->i_dev = dev;
                inode->i_dirt = 1;
                inode->i_atime = inode->i_ctime = inode->i_mtime = get_current_time();
                inode->i_sb = vsb;

                brelse(buf);
                return inode;
            }
        }
        brelse(buf);
    }
    return NULL;
}
