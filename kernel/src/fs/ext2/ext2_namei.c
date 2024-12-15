#include <ext2.h>
#include <process.h>
#include <string.h>
#include <panic.h>
#include <blk_dev.h>
#include <heap.h>
#include <timer.h>
#include <errno.h>
#include <fcntl.h>
#include <vfs.h>


static i32 bmap(struct vfs_inode *inode, u32 offset, i32 create) {
	struct buffer *buf;
	u32 block, res_block, ind_block, dind_block, ind_block2;

	block = offset / inode->i_sb->s_block_size;
	if (block < 12) {
		if (!inode->u.i_ext2.i_block[block] && create) {
			inode->u.i_ext2.i_block[block] = alloc_block(inode->i_dev);
			inode->i_blocks += 2;
			inode->i_dirt = 1;
			inode->i_ctime = get_current_time();
			inode->i_size += inode->i_sb->s_block_size;
		}
		debug("Direct block\r\n");
		return inode->u.i_ext2.i_block[block];
	} else if (block < 12 + EXT2_POINTERS_PER_BLOCK) {
		debug("Indirect block\r\n");
		block -= 12;
		if (!inode->u.i_ext2.i_block[12] && create) {
			inode->u.i_ext2.i_block[block] = alloc_block(inode->i_dev);
			inode->i_blocks += 2;
			inode->i_dirt = 1;
			inode->i_ctime = get_current_time();
			inode->i_size += inode->i_sb->s_block_size;
		}
		buf = read_blk(inode->i_dev, inode->u.i_ext2.i_block[12]);
		res_block = ((u32 *)buf->b_data)[block];
		if (!res_block && create) {
			((u32 *)buf->b_data)[block] = res_block = alloc_block(inode->i_dev);
			inode->i_blocks += 2;
			inode->i_dirt = 1;
			inode->i_ctime = get_current_time();
			inode->i_size += inode->i_sb->s_block_size;
		}
		free(buf->b_data);
		free(buf);
		return res_block;
	} else if (block < 12 + EXT2_POINTERS_PER_BLOCK + EXT2_POINTERS_PER_BLOCK *
			EXT2_POINTERS_PER_BLOCK) {
		debug("Doubly-indirect block\r\n");
		block -= 12;
		block -= EXT2_POINTERS_PER_BLOCK;
		if (!inode->u.i_ext2.i_block[13] && create) {
			inode->u.i_ext2.i_block[block] = alloc_block(inode->i_dev);
			inode->i_blocks += 2;
			inode->i_dirt = 1;
			inode->i_ctime = get_current_time();
			inode->i_size += inode->i_sb->s_block_size;
		}
		buf = read_blk(inode->i_dev, inode->u.i_ext2.i_block[13]);
		ind_block = ((u32 *)buf->b_data)[block / EXT2_POINTERS_PER_BLOCK];
		if (!ind_block && create) {
			((u32 *)buf->b_data)[block / EXT2_POINTERS_PER_BLOCK] =
				ind_block = alloc_block(inode->i_dev);
			inode->i_blocks += 2;
			inode->i_dirt = 1;
			inode->i_ctime = get_current_time();
			inode->i_size += inode->i_sb->s_block_size;
		}
		free(buf->b_data);
		free(buf);
		buf = read_blk(inode->i_dev, ind_block);
		res_block = ((u32 *)buf->b_data)[block % EXT2_POINTERS_PER_BLOCK];
		if (!res_block && create) {
			((u32 *)buf->b_data)[block] = res_block = alloc_block(inode->i_dev);
			inode->i_blocks += 2;
			inode->i_dirt = 1;
			inode->i_ctime = get_current_time();
			inode->i_size += inode->i_sb->s_block_size;
		}
		free(buf->b_data);
		free(buf);
		return res_block;
	} else if (block < 12 + EXT2_POINTERS_PER_BLOCK + EXT2_POINTERS_PER_BLOCK *
			EXT2_POINTERS_PER_BLOCK + EXT2_POINTERS_PER_BLOCK *
			EXT2_POINTERS_PER_BLOCK * EXT2_POINTERS_PER_BLOCK) {
		debug("Triply-indirect block\r\n");
		block -= 12;
		block -= EXT2_POINTERS_PER_BLOCK;
		block -= EXT2_POINTERS_PER_BLOCK * EXT2_POINTERS_PER_BLOCK;
		if (!inode->u.i_ext2.i_block[14] && create) {
			inode->u.i_ext2.i_block[block] = alloc_block(inode->i_dev);
			inode->i_blocks += 2;
			inode->i_dirt = 1;
			inode->i_ctime = get_current_time();
			inode->i_size += inode->i_sb->s_block_size;
		}
		buf = read_blk(inode->i_dev, inode->u.i_ext2.i_block[14]);
		dind_block = ((u32 *)buf->b_data)[block / (EXT2_POINTERS_PER_BLOCK * EXT2_POINTERS_PER_BLOCK)];
		if (!dind_block && create) {
			((u32 *)buf->b_data)[block / (EXT2_POINTERS_PER_BLOCK * EXT2_POINTERS_PER_BLOCK)] =
				dind_block = alloc_block(inode->i_dev);
			inode->i_blocks += 2;
			inode->i_dirt = 1;
			inode->i_ctime = get_current_time();
			inode->i_size += inode->i_sb->s_block_size;
		}
		free(buf->b_data);
		free(buf);
		buf = read_blk(inode->i_dev, dind_block);
		ind_block = ((u32 *)buf->b_data)[block % (EXT2_POINTERS_PER_BLOCK * EXT2_POINTERS_PER_BLOCK)];
		if (!ind_block && create) {
			((u32 *)buf->b_data)[block % (EXT2_POINTERS_PER_BLOCK * EXT2_POINTERS_PER_BLOCK)] =
				ind_block = alloc_block(inode->i_dev);
			inode->i_blocks += 2;
			inode->i_dirt = 1;
			inode->i_ctime = get_current_time();
			inode->i_size += inode->i_sb->s_block_size;
		}
		ind_block2 = ind_block / EXT2_POINTERS_PER_BLOCK;
		free(buf->b_data);
		free(buf);
		buf = read_blk(inode->i_dev, ind_block2);
		res_block = ((u32 *)buf->b_data)[ind_block % EXT2_POINTERS_PER_BLOCK];
		if (!res_block && create) {
			((u32 *)buf->b_data)[ind_block % EXT2_POINTERS_PER_BLOCK] =
				res_block = alloc_block(inode->i_dev);
			inode->i_blocks += 2;
			inode->i_dirt = 1;
			inode->i_ctime = get_current_time();
			inode->i_size += inode->i_sb->s_block_size;
		}
		free(buf->b_data);
		free(buf);
		return res_block;
	}
	return 0;
}

i32 ext2_bmap(struct vfs_inode *inode, u32 offset) {
	return bmap(inode, offset, 0);
}

i32 ext2_create_block(struct vfs_inode *inode, u32 offset) {
	return bmap(inode, offset, 1);
}

i32 ext2_add_entry(struct vfs_inode *dir, const i8 *name,
		struct buffer **res_buf, struct ext2_dir **result) {
	struct buffer *buf;
	struct ext2_dir *de, *de1;
	u32 block, rec_len, len;
	i32 curr_off, inblock_offset;

	*res_buf = NULL;
	len = strlen(name);
	if (len > EXT2_NAME_LEN) {
		return -ENAMETOOLONG;
	}
	*result = NULL;
	if (!dir) {
		return -ENOENT;
	}
	curr_off = inblock_offset = 0;
	if (!(block = dir->u.i_ext2.i_block[0])) {
		return -ENOSPC;
	}
	rec_len = EXT2_DIR_REC_LEN(len);
	buf = read_blk(dir->i_dev, block);
	if (!buf) {
		return -ENOSPC;
	}
	while (1) {
		if (inblock_offset >= (i32)dir->i_sb->s_block_size) {
			free(buf->b_data);
			free(buf);
			inblock_offset = 0;
			if ((u32)curr_off >= dir->i_size) {
				block = ext2_create_block(dir, curr_off);
				if (!block) {
					*res_buf = NULL;
					return -ENOSPC;
				}
				buf = read_blk(dir->i_dev, block);
				if (!buf) {
					return -ENOSPC;
				}
				dir->i_size += dir->i_sb->s_block_size;
				de = (struct ext2_dir *)(buf->b_data + inblock_offset);
				de->inode = 0;
				de->rec_len = dir->i_sb->s_block_size;
				write_blk(buf);
			} else {
				block = ext2_bmap(dir, curr_off);
				if (!block) {
					*res_buf = NULL;
					return -ENOSPC;
				}
				buf = read_blk(dir->i_dev, block);
				if (!buf) {
					return -ENOSPC;
				}
			}
		}
		de = (struct ext2_dir *)(buf->b_data + inblock_offset);
		if (de->inode != 0 && de->name_len == strlen(name) && strncmp(name, de->name, de->name_len) == 0) {
			free(buf->b_data);
			free(buf);
			*res_buf = NULL;
			return -EEXIST;
		}
		if ((de->inode == 0 && de->rec_len >= rec_len) || (de->rec_len >= EXT2_DIR_REC_LEN(de->name_len) + rec_len)) {
			if (de->inode) {
				de1 = (struct ext2_dir *)(buf->b_data + inblock_offset + EXT2_DIR_REC_LEN(de->name_len));
				de1->rec_len = de->rec_len - EXT2_DIR_REC_LEN(de->name_len);
				de->rec_len = EXT2_DIR_REC_LEN(de->name_len);
				de = de1;
			}
			de->inode = 0;
			de->name_len = len;
			memcpy(de->name, name, len);
			dir->i_mtime = dir->i_ctime = get_current_time();
			dir->i_dirt = 1;
			write_blk(buf);
			*result = de;
			*res_buf = buf;
			return 0;
		}
		inblock_offset += de->rec_len;
		curr_off += de->rec_len;
	}
	*res_buf = NULL;
	return -EINVAL;
}

i32 ext2_create(struct vfs_inode *dir, const i8 *name, i32 mode,
		struct vfs_inode **result) {
	i32 err;
	struct buffer *buf;
	struct vfs_inode *inode;
	struct ext2_dir *de;

	*result = NULL;
	if (!dir) {
		return -ENOENT;
	}
	inode = alloc_inode(dir->i_dev);
	if (!inode) {
		vfs_iput(dir);
		return -ENOSPC;
	}
	inode->i_mode = EXT2_S_IFREG | (mode & 0777 & ~current_process->umask);
	inode->i_dirt = 1;
	if ((err = ext2_add_entry(dir, name, &buf, &de))) {
		--inode->i_links_count;
		vfs_iput(inode);
		vfs_iput(dir);
		return err;
	}
	de->inode = inode->i_num;
	write_blk(buf);
	free(buf->b_data);
	free(buf);
	vfs_iput(dir);
	*result = inode;
	return 0;
}

struct buffer *ext2_find_entry(struct vfs_inode *dir, const i8 *name,
		struct ext2_dir **res_dir, struct ext2_dir **prev_dir) {
	struct buffer *buf;
	struct ext2_dir *de;
	i32 curr_off, inblock_offset;
	u32 block;

	*res_dir = NULL;
	if (!dir) {
		return NULL;
	}
	if (strlen(name) > EXT2_NAME_LEN) {
		return NULL;
	}
	curr_off = inblock_offset = 0;
	if (!(block = dir->u.i_ext2.i_block[0])) {
		return NULL;
	}
	buf = read_blk(dir->i_dev, block);
	if (!buf) {
		return NULL;
	}
	if (prev_dir) {
		*prev_dir = NULL;
	}
	while ((u32)curr_off < dir->i_size) {
		if (inblock_offset >= (i32)dir->i_sb->s_block_size) {
			block = ext2_bmap(dir, curr_off);
			inblock_offset = 0;
			free(buf->b_data);
			free(buf);
			if (!block) {
				curr_off += BLOCK_SIZE;
				continue;
			}
			buf = read_blk(dir->i_dev, block);
			if (!buf) {
				curr_off += BLOCK_SIZE;
				continue;
			}
		}
		de = (struct ext2_dir *)(buf->b_data + inblock_offset);
		if (de->inode && de->name_len == strlen(name) && strncmp(name, de->name, de->name_len) == 0) {
			*res_dir = de;
			return buf;
		}
		if (prev_dir && de->inode) {
			*prev_dir = de;
		}
		inblock_offset += de->rec_len;
		curr_off += de->rec_len;
	}
	return NULL;
}

/* Its searches the inode by 'name' entry in VFS directory inode 'dir' */
i32 ext2_lookup(struct vfs_inode *dir, const i8 *name, struct vfs_inode **res) {
	struct buffer *buf;
	u32 inr;
	struct ext2_dir *de;

	*res = NULL;
	if (!dir) {
		return -ENOENT;
	}
	if (!check_permission(dir, MAY_EXEC)) {
		vfs_iput(dir);
		return -EACCES;
	}
	if (*name == '\0') {
		*res = dir;
		return 0;
	}
	if (!EXT2_S_ISDIR(dir->i_mode)) {
		vfs_iput(dir);
		return -ENOTDIR;
	}
	if (strlen(name) > EXT2_NAME_LEN) {
		vfs_iput(dir);
		return -ENAMETOOLONG;
	}
	if (!(buf = ext2_find_entry(dir, name, &de, NULL))) {
		vfs_iput(dir);
		return -ENOENT;
	}
	inr = de->inode;
	free(buf->b_data);
	free(buf);
	if (!(*res = vfs_iget(dir->i_dev, inr))) {
		vfs_iput(dir);
		return -EACCES;
	}
	vfs_iput(dir);
	return 0;
}

i32 ext2_delete_entry(struct ext2_dir *dir, struct buffer *old_buf) {
	struct ext2_dir *de, *pde;
	i32 curr_off = 0;
	pde = NULL;
	de = (struct ext2_dir *)old_buf->b_data;
	while (curr_off < 1024) {
		if (de == dir) {
			if (pde) {
				pde->rec_len += dir->rec_len;
			} else {
				dir->inode = 0;
			}
			return 0;
		}
		curr_off += de->rec_len;
		pde = de;
		de = (struct ext2_dir *)(old_buf->b_data + curr_off);
	}
	return -ENOENT;
}

