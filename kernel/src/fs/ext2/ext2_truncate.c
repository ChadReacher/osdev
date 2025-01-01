#include <ext2.h>
#include <blk_dev.h>
#include <timer.h>
#include <heap.h>
#include <vfs.h>

static void trunc_direct(struct vfs_inode *inode) {
	struct buffer *buf;
	u32 i;
	for (i = 0; i < 12; ++i) {
		if (!inode->u.i_ext2.i_block[i]) {
			continue;
		}
		buf = read_blk(inode->i_dev, inode->u.i_ext2.i_block[i]);
		memset(buf->b_data, 0, 1024);
		write_blk(buf);
		free_block(inode->i_dev, inode->u.i_ext2.i_block[i]);
		inode->u.i_ext2.i_block[i] = 0;
		inode->i_blocks -= 2;
	}
}

static void trunc_indirect(struct vfs_inode *inode) {
	struct buffer *buf;
	u32 i, *blocks;

	if (!inode->u.i_ext2.i_block[12]) {
		return;
	}
	buf = read_blk(inode->i_dev, inode->u.i_ext2.i_block[12]);
	blocks = (u32 *)buf->b_data;
	for (i = 0; i < 256; ++i) {
		if (!blocks[i]) {
			continue;
		}
		free_block(inode->i_dev, blocks[i]);
		blocks[i] = 0;
		inode->i_blocks -= 2;
	}
	write_blk(buf);
	free_block(inode->i_dev, inode->u.i_ext2.i_block[12]);
	inode->u.i_ext2.i_block[12] = 0;
	inode->i_blocks -= 2;
	free(buf->b_data);
	free(buf);
}

static void trunc_doubly_indirect(struct vfs_inode *inode) {
	u32 i, j;
	struct buffer *buf, *buf2;
	u32 *indirect, *blocks;

	if (!inode->u.i_ext2.i_block[13]) {
		return;
	}
	buf = read_blk(inode->i_dev, inode->u.i_ext2.i_block[13]);
	indirect = (u32 *)buf->b_data;
	for (i = 0; i < 256; ++i) {
		if (!indirect[i]) {
			continue;
		}
		buf2 = read_blk(inode->i_dev, indirect[i]);
		blocks = (u32 *)buf2->b_data;
		for (j = 0; j < 256; ++j) {
			if (!blocks[j]) {
				continue;
			}
			free_block(inode->i_dev, blocks[j]);
			blocks[j] = 0;
			inode->i_blocks -= 2;
		}
		write_blk(buf2);

		free_block(inode->i_dev, indirect[i]);
		indirect[i] = 0;
		inode->i_blocks -= 2;

		free(buf2->b_data);
		free(buf2);
	}
	write_blk(buf);
	free_block(inode->i_dev, inode->u.i_ext2.i_block[13]);
	inode->u.i_ext2.i_block[13] = 0;
	inode->i_blocks -= 2;
	free(buf->b_data);
	free(buf);
}

static void trunc_triply_indirect(struct vfs_inode *inode) {
	struct buffer *buf, *buf2, *buf3;
	u32 *dindirect, *indirect, *blocks;
	u32 i, j, k;

	if (!inode->u.i_ext2.i_block[14]) {
		return;
	}
	buf = read_blk(inode->i_dev, inode->u.i_ext2.i_block[14]);
	dindirect = (u32 *)buf->b_data;
	for (i = 0; i < 256; ++i) {
		if (!dindirect[i]) {
			continue;
		}
		buf2 = read_blk(inode->i_dev, dindirect[i]);
		indirect = (u32 *)buf2->b_data;
		for (j = 0; j < 256; ++j) {
			if (!indirect[j]) {
				continue;
			}
			buf3 = read_blk(inode->i_dev, indirect[j]);
			blocks = (u32 *)buf3->b_data;
			for (k = 0; k < 256; ++k) {
				if (!blocks[k]) {
					continue;
				}
				free_block(inode->i_dev, blocks[k]);
				blocks[k] = 0;
				inode->i_blocks -= 2;
			}
			write_blk(buf3);
			free_block(inode->i_dev, indirect[j]);
			indirect[j] = 0;
			inode->i_blocks -= 2;

			free(buf3->b_data);
			free(buf3);
		}
		write_blk(buf2);
		free_block(inode->i_dev, dindirect[i]);
		dindirect[i] = 0;
		inode->i_blocks -= 2;

		free(buf2->b_data);
		free(buf2);
	}
	write_blk(buf);
	free_block(inode->i_dev, inode->u.i_ext2.i_block[14]);
	inode->u.i_ext2.i_block[14] = 0;
	inode->i_blocks -= 2;

	free(buf->b_data);
	free(buf);
}

/* TODO: For now we assume truncate to size 0 */
i32 ext2_truncate(struct vfs_inode *inode) {
	trunc_direct(inode);
	trunc_indirect(inode);
	trunc_doubly_indirect(inode);
	trunc_triply_indirect(inode);
	inode->i_size = 0;
	inode->i_dirt = 1;
	inode->i_mtime = inode->i_ctime = get_current_time();
	return 0;
}

