#include <ext2.h>
#include <blk_dev.h>
#include <timer.h>

extern struct ext2_super_block super_block;

void free_block(u16 dev, u32 block) {
	i8 *buf;
	u32 *buf2;
	struct ext2_blk_grp_desc *bgd; 
	u32 bitmap_block;

	u32 group = block / super_block.s_blocks_per_group;
	bgd = malloc(sizeof(struct ext2_blk_grp_desc));
	read_group_desc(bgd, group);
	if (!bgd) {
		return;
	}
	bitmap_block = bgd->bg_block_bitmap;

	rw_block(READ, dev, bitmap_block, &buf);
	buf2 = (u32 *)buf;

	u32 sub_bitmap_idx = (block - super_block.s_blocks_per_group * group) / 32; 
	u32 idx = (block - super_block.s_blocks_per_group * group) % 32;
	--idx;
	
	u32 mask = ~(1 << idx);
	buf2[sub_bitmap_idx] = buf2[sub_bitmap_idx] & mask;
	
	rw_block(WRITE, dev, bitmap_block, &buf);

	++bgd->bg_free_blocks_count;
	++super_block.s_free_blocks_count;
	write_group_desc(bgd, group);

	free(bgd);
	free(buf);
}

u32 alloc_block(u16 dev) {
	i8 *buf;
	u32 *buf2;
	struct ext2_blk_grp_desc *bgd;
	u32 bitmap_block;

	for (u32 i = 0; i < super_block.s_total_groups; ++i) {
		bgd = malloc(sizeof(struct ext2_blk_grp_desc));
		read_group_desc(bgd, i);
		if (!bgd->bg_free_blocks_count) {
			continue;
		}
		bitmap_block = bgd->bg_block_bitmap;
		rw_block(READ, dev, bitmap_block, &buf);
		buf2 = (u32 *)buf;
		for (u32 j = 0; j < super_block.s_block_size / 4; ++j) {
			u32 sub_bitmap = buf2[j];
			if (sub_bitmap == 0xFFFFFFFF) {
				continue;
			}
			for (u32 k = 0; k < 32; ++k) {
				u32 is_free = !(sub_bitmap & (1 << k));
				if (is_free) {
					u32 mask = (1 << k);
					buf2[j] = (buf2[j] | mask);
					rw_block(WRITE, dev, bitmap_block, &buf);
					--bgd->bg_free_blocks_count;
					write_group_desc(bgd, i);
					free(buf);
					return i * super_block.s_blocks_per_group + j * 32 + k + 1;
				}
			}
		}
		free(buf);
	}
	free(buf);
	return 0;
}

void free_inode(struct ext2_inode *inode) {
	i8 *buf;
	u32 *buf2;
	struct ext2_blk_grp_desc *bgd; 
	u32 bitmap_inode;

	u32 group = inode->i_num / super_block.s_inodes_per_group;
	bgd = malloc(sizeof(struct ext2_blk_grp_desc));
	read_group_desc(bgd, group);
	if (!bgd) {
		return;
	}
	bitmap_inode = bgd->bg_inode_bitmap;

	rw_block(READ, inode->i_dev, bitmap_inode, &buf);
	buf2 = (u32 *)buf;

	u32 sub_bitmap_idx = (inode->i_num - super_block.s_inodes_per_group * group) / 32; 
	u32 idx = (inode->i_num - super_block.s_inodes_per_group * group) % 32;
	--idx;
	
	u32 mask = ~(1 << idx);
	buf2[sub_bitmap_idx] = buf2[sub_bitmap_idx] & mask;
	
	rw_block(WRITE, inode->i_dev, bitmap_inode, &buf);

	++bgd->bg_free_inodes_count;
	++super_block.s_free_inodes_count;
	write_group_desc(bgd, group);

	memset(inode, 0, sizeof(struct ext2_inode));
	free(bgd);
	free(buf);
}

struct ext2_inode *alloc_inode(u16 dev) {
	i8 *buf;
	u32 *buf2;
	struct ext2_blk_grp_desc *bgd; 
	struct ext2_inode *inode;
	u32 bitmap_block;

	inode = get_empty_inode();

	for (u32 i = 0; i < super_block.s_total_groups; ++i) {
		bgd = malloc(sizeof(struct ext2_blk_grp_desc));
		read_group_desc(bgd, i);
		if (!bgd->bg_free_blocks_count) {
			continue;
		}
		bitmap_block = bgd->bg_inode_bitmap;
		rw_block(READ, dev, bitmap_block, &buf);
		buf2 = (u32 *)buf;
		for (u32 j = 0; j < super_block.s_block_size / 4; ++j) {
			u32 sub_bitmap = buf2[j];
			if (sub_bitmap == 0xFFFFFFFF) {
				continue;
			}
			for (u32 k = 0; k < 32; ++k) {
				u32 is_free = !(sub_bitmap & (1 << k));
				if (is_free) {
					u32 mask = 1 << k;
					buf2[j] = buf2[j] | mask;
					rw_block(WRITE, dev, bitmap_block, &buf);
					--bgd->bg_free_inodes_count;
					
					write_group_desc(bgd, i);
					free(buf);
					inode->i_num = i * super_block.s_inodes_per_group + j * 32 + k + 1;
					goto new_inr;
				}
			}
		}
		free(buf);
	}
new_inr:
	inode->i_count = 1;
	inode->i_links_count = 1;
	inode->i_dev = dev;
	inode->i_dirt = 1;
	inode->i_atime = inode->i_ctime = inode->i_mtime = inode->i_dtime = get_current_time();
	return inode;
}
