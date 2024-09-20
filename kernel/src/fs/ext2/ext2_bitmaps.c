#include <ext2.h>
#include <blk_dev.h>
#include <timer.h>
#include <heap.h>
#include <string.h>
#include <vfs.h>

void free_block(u16 dev, u32 block) {
	struct buffer *buf;
	u32 *buf2;
	struct ext2_blk_grp_desc *bgd; 
	u32 bitmap_block;
	u32 sub_bitmap_idx, idx, mask;
	struct vfs_superblock *vsb = get_vfs_super(dev);

	u32 group = block / vsb->u.ext2_sb.s_blocks_per_group;
	bgd = malloc(sizeof(struct ext2_blk_grp_desc));
	read_group_desc(bgd, group, vsb);
	if (!bgd) {
		return;
	}
	bitmap_block = bgd->bg_block_bitmap;

	buf = read_blk(dev, bitmap_block);
	buf2 = (u32 *)buf->b_data;

	sub_bitmap_idx = (block - vsb->u.ext2_sb.s_blocks_per_group * group) / 32; 
	idx = (block - vsb->u.ext2_sb.s_blocks_per_group * group) % 32;
	--idx;
	
	mask = ~(1 << idx);
	buf2[sub_bitmap_idx] = buf2[sub_bitmap_idx] & mask;
	
	write_blk(buf);

	++bgd->bg_free_blocks_count;
	++vsb->u.ext2_sb.s_free_blocks_count;
	write_group_desc(bgd, group, vsb);

	free(bgd);
	free(buf->b_data);
	free(buf);
}

u32 alloc_block(u16 dev) {
	struct buffer *buf;
	u32 *buf2;
	struct ext2_blk_grp_desc *bgd;
	u32 bitmap_block;
	struct vfs_superblock *vsb = get_vfs_super(dev);

	u32 i, j, k;
	u32 s_total_groups = vsb->u.ext2_sb.s_blocks_count / vsb->u.ext2_sb.s_blocks_per_group;
	for (i = 0; i < s_total_groups; ++i) {
		bgd = malloc(sizeof(struct ext2_blk_grp_desc));
		read_group_desc(bgd, i, vsb);
		if (!bgd->bg_free_blocks_count) {
			continue;
		}
		bitmap_block = bgd->bg_block_bitmap;
		buf = read_blk(dev, bitmap_block);
		buf2 = (u32 *)buf->b_data;
		for (j = 0; j < vsb->s_block_size / 4; ++j) {
			u32 sub_bitmap = buf2[j];
			if (sub_bitmap == 0xFFFFFFFF) {
				continue;
			}
			for (k = 0; k < 32; ++k) {
				u32 is_free = !(sub_bitmap & (1 << k));
				if (is_free) {
					u32 mask = 1 << k;
					buf2[j] = buf2[j] | mask;
					write_blk(buf);

					--bgd->bg_free_blocks_count;
					write_group_desc(bgd, i, vsb);
					free(buf->b_data);
					free(buf);
					return i * vsb->u.ext2_sb.s_blocks_per_group + j * 32 + k + 1;
				}
			}
		}
		free(buf->b_data);
		free(buf);
	}
	return 0;
}

void ext2_free_inode(struct vfs_inode *inode) {
	struct buffer *buf;
	u32 *buf2;
	struct ext2_blk_grp_desc *bgd; 
	u32 bitmap_inode;
	u32 sub_bitmap_idx, idx, mask;

	struct vfs_superblock *vsb = inode->i_sb;
	u32 group = inode->i_num / vsb->u.ext2_sb.s_inodes_per_group;
	bgd = malloc(sizeof(struct ext2_blk_grp_desc));
	read_group_desc(bgd, group, vsb);
	if (!bgd) {
		return;
	}
	bitmap_inode = bgd->bg_inode_bitmap;

	buf = read_blk(inode->i_dev, bitmap_inode);
	buf2 = (u32 *)buf;

	sub_bitmap_idx = (inode->i_num - vsb->u.ext2_sb.s_inodes_per_group * group) / 32; 
	idx = (inode->i_num - vsb->u.ext2_sb.s_inodes_per_group * group) % 32;
	--idx;
	
	mask = ~(1 << idx);
	buf2[sub_bitmap_idx] = buf2[sub_bitmap_idx] & mask;
	
	write_blk(buf);

	++bgd->bg_free_inodes_count;
	++vsb->u.ext2_sb.s_free_inodes_count;
	write_group_desc(bgd, group, vsb);

	memset(inode, 0, sizeof(struct ext2_inode));
	free(bgd);
	free(buf->b_data);
	free(buf);
}

struct vfs_inode *alloc_inode(u16 dev) {
	struct buffer *buf;
	u32 *buf2;
	struct ext2_blk_grp_desc *bgd; 
	struct vfs_inode *inode;
	u32 bitmap_block;
	u32 i, j, k;
	struct vfs_superblock *vsb = get_vfs_super(dev);

	inode = get_empty_inode();

	u32 s_total_groups = vsb->u.ext2_sb.s_blocks_count / vsb->u.ext2_sb.s_blocks_per_group;
	for (i = 0; i < s_total_groups; ++i) {
		bgd = malloc(sizeof(struct ext2_blk_grp_desc));
		read_group_desc(bgd, i, vsb);
		if (!bgd->bg_free_inodes_count) {
			continue;
		}
		bitmap_block = bgd->bg_inode_bitmap;
		buf = read_blk(dev, bitmap_block);
		buf2 = (u32 *)buf->b_data;
		for (j = 0; j < vsb->s_block_size / 4; ++j) {
			u32 sub_bitmap = buf2[j];
			if (sub_bitmap == 0xFFFFFFFF) {
				continue;
			}
			for (k = 0; k < 32; ++k) {
				u32 is_free = !(sub_bitmap & (1 << k));
				if (is_free) {
					u32 mask = 1 << k;
					buf2[j] = buf2[j] | mask;
					write_blk(buf);

					--bgd->bg_free_inodes_count;
					write_group_desc(bgd, i, vsb);
					free(buf->b_data);
					free(buf);
					inode->i_num = i * vsb->u.ext2_sb.s_inodes_per_group + j * 32 + k + 1;
					inode->i_count = 1;
					inode->i_links_count = 1;
					inode->i_dev = dev;
					inode->i_dirt = 1;
					inode->i_atime = inode->i_ctime = inode->i_mtime = get_current_time();
					inode->i_sb = vsb;
					return inode;
				}
			}
		}
		free(buf->b_data);
		free(buf);
	}
	return NULL;
}
