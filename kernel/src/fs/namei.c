#include <ext2.h>
#include <process.h>
#include <string.h>
#include <debug.h>
#include <panic.h>
#include <blk_dev.h>
#include <heap.h>
#include <timer.h>
#include <errno.h>
#include <fcntl.h>

extern process_t *current_process;

i32 check_permission(struct ext2_inode *inode, i32 mask) {
	i32 mode = inode->i_mode;
	if (inode->i_dev && !inode->i_links_count) {
		return 0;
	}
	if (!(current_process->uid && current_process->euid)) {
		mode = 0777;
	} else if (current_process->uid == inode->i_uid ||
			current_process->euid == inode->i_uid) {
		mode >>= 6;
	} else if (current_process->gid == inode->i_gid ||
			current_process->egid == inode->i_gid) {
		mode >>= 3;
	}
	return mode & 0007 & mask;
}

static i32 bmap(struct ext2_inode *inode, u32 offset, i32 create) {
	struct buffer *buf;
	u32 block;

	block = offset / super_block.s_block_size;
	if (block < 12) {
		if (!inode->i_block[block] && create) {
			inode->i_block[block] = alloc_block(inode->i_dev);
			inode->i_blocks += 2;
			inode->i_dirt = 1;
			inode->i_ctime = get_current_time();
			inode->i_size += super_block.s_block_size;
		}
		DEBUG("Direct block\r\n");
		return inode->i_block[block];
	} else if (block < 12 + EXT2_POINTERS_PER_BLOCK) {
		DEBUG("Indirect block\r\n");
		block -= 12;
		if (!inode->i_block[12] && create) {
			inode->i_block[block] = alloc_block(inode->i_dev);
			inode->i_blocks += 2;
			inode->i_dirt = 1;
			inode->i_ctime = get_current_time();
			inode->i_size += super_block.s_block_size;
		}
		buf = read_blk(inode->i_dev, inode->i_block[12]);
		//rw_block(READ, inode->i_dev, inode->i_block[12], &buf);
		u32 res_block = ((u32 *)buf->b_data)[block];
		if (!res_block && create) {
			((u32 *)buf->b_data)[block] = res_block = alloc_block(inode->i_dev);
			inode->i_blocks += 2;
			inode->i_dirt = 1;
			inode->i_ctime = get_current_time();
			inode->i_size += super_block.s_block_size;
		}
		free(buf->b_data);
		free(buf);
		return res_block;
	} else if (block < 12 + EXT2_POINTERS_PER_BLOCK + EXT2_POINTERS_PER_BLOCK *
			EXT2_POINTERS_PER_BLOCK) {
		DEBUG("Doubly-indirect block\r\n");
		block -= 12;
		block -= EXT2_POINTERS_PER_BLOCK;
		if (!inode->i_block[13] && create) {
			inode->i_block[block] = alloc_block(inode->i_dev);
			inode->i_blocks += 2;
			inode->i_dirt = 1;
			inode->i_ctime = get_current_time();
			inode->i_size += super_block.s_block_size;
		}
		buf = read_blk(inode->i_dev, inode->i_block[13]);
		//rw_block(READ, inode->i_dev, inode->i_block[13], &buf);
		u32 ind_block = ((u32 *)buf->b_data)[block / EXT2_POINTERS_PER_BLOCK];
		if (!ind_block && create) {
			((u32 *)buf->b_data)[block / EXT2_POINTERS_PER_BLOCK] =
				ind_block = alloc_block(inode->i_dev);
			inode->i_blocks += 2;
			inode->i_dirt = 1;
			inode->i_ctime = get_current_time();
			inode->i_size += super_block.s_block_size;
		}
		free(buf->b_data);
		free(buf);
		buf = read_blk(inode->i_dev, ind_block);
		//rw_block(READ, inode->i_dev, ind_block, &buf);
		u32 res_block = ((u32 *)buf->b_data)[block % EXT2_POINTERS_PER_BLOCK];
		if (!res_block && create) {
			((u32 *)buf->b_data)[block] = res_block = alloc_block(inode->i_dev);
			inode->i_blocks += 2;
			inode->i_dirt = 1;
			inode->i_ctime = get_current_time();
			inode->i_size += super_block.s_block_size;
		}
		free(buf->b_data);
		free(buf);
		return res_block;
	} else if (block < 12 + EXT2_POINTERS_PER_BLOCK + EXT2_POINTERS_PER_BLOCK *
			EXT2_POINTERS_PER_BLOCK + EXT2_POINTERS_PER_BLOCK *
			EXT2_POINTERS_PER_BLOCK * EXT2_POINTERS_PER_BLOCK) {
		DEBUG("Triply-indirect block\r\n");
		block -= 12;
		block -= EXT2_POINTERS_PER_BLOCK;
		block -= EXT2_POINTERS_PER_BLOCK * EXT2_POINTERS_PER_BLOCK;
		if (!inode->i_block[14] && create) {
			inode->i_block[block] = alloc_block(inode->i_dev);
			inode->i_blocks += 2;
			inode->i_dirt = 1;
			inode->i_ctime = get_current_time();
			inode->i_size += super_block.s_block_size;
		}
		buf = read_blk(inode->i_dev, inode->i_block[14]);
		//rw_block(READ, inode->i_dev, inode->i_block[14], &buf);
		u32 dind_block = ((u32 *)buf->b_data)[block / (EXT2_POINTERS_PER_BLOCK * EXT2_POINTERS_PER_BLOCK)];
		if (!dind_block && create) {
			((u32 *)buf->b_data)[block / (EXT2_POINTERS_PER_BLOCK * EXT2_POINTERS_PER_BLOCK)] =
				dind_block = alloc_block(inode->i_dev);
			inode->i_blocks += 2;
			inode->i_dirt = 1;
			inode->i_ctime = get_current_time();
			inode->i_size += super_block.s_block_size;
		}
		free(buf->b_data);
		free(buf);
		buf = read_blk(inode->i_dev, dind_block);
		//rw_block(READ, inode->i_dev, dind_block, &buf);
		u32 ind_block = ((u32 *)buf->b_data)[block % (EXT2_POINTERS_PER_BLOCK * EXT2_POINTERS_PER_BLOCK)];
		if (!ind_block && create) {
			((u32 *)buf->b_data)[block % (EXT2_POINTERS_PER_BLOCK * EXT2_POINTERS_PER_BLOCK)] =
				ind_block = alloc_block(inode->i_dev);
			inode->i_blocks += 2;
			inode->i_dirt = 1;
			inode->i_ctime = get_current_time();
			inode->i_size += super_block.s_block_size;
		}
		u32 ind_block2 = ind_block / EXT2_POINTERS_PER_BLOCK;
		free(buf->b_data);
		free(buf);
		buf = read_blk(inode->i_dev, ind_block2);
		//rw_block(READ, inode->i_dev, ind_block2, &buf);
		u32 res_block = ((u32 *)buf->b_data)[ind_block % EXT2_POINTERS_PER_BLOCK];
		if (!res_block && create) {
			((u32 *)buf->b_data)[ind_block % EXT2_POINTERS_PER_BLOCK] =
				res_block = alloc_block(inode->i_dev);
			inode->i_blocks += 2;
			inode->i_dirt = 1;
			inode->i_ctime = get_current_time();
			inode->i_size += super_block.s_block_size;
		}
		free(buf->b_data);
		free(buf);
		return res_block;
	}
	return 0;
}

i32 ext2_bmap(struct ext2_inode *inode, u32 offset) {
	return bmap(inode, offset, 0);
}

i32 ext2_create_block(struct ext2_inode *inode, u32 offset) {
	return bmap(inode, offset, 1);
}

i32 ext2_add_entry(struct ext2_inode *dir, const i8 *name,
		struct buffer **res_buf, struct ext2_dir **result) {
	struct buffer *buf;
	struct ext2_dir *de, *de1;
	u32 curr_off, block, inblock_offset, rec_len, len;

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
	if (!(block = dir->i_block[0])) {
		return -ENOSPC;
	}
	rec_len = EXT2_DIR_REC_LEN(len);
	buf = read_blk(dir->i_dev, block);
	//rw_block(READ, dir->i_dev, block, &buf);
	if (!buf) {
		return -ENOSPC;
	}
	while (1) {
		if (inblock_offset >= super_block.s_block_size) {
			free(buf->b_data);
			free(buf);
			inblock_offset = 0;
			if (curr_off >= dir->i_size) {
				block = ext2_create_block(dir, curr_off);
				if (!block) {
					*res_buf = NULL;
					return -ENOSPC;
				}
				buf = read_blk(dir->i_dev, block);
				//rw_block(READ, dir->i_dev, block, &buf);
				if (!buf) {
					return -ENOSPC;
				}
				de = (struct ext2_dir *)(buf->b_data + inblock_offset);
				de->inode = 0;
				de->rec_len = super_block.s_block_size;
				write_blk(buf);
				//rw_block(WRITE, dir->i_dev, block, &buf);
			} else {
				block = ext2_bmap(dir, curr_off);
				if (!block) {
					*res_buf = NULL;
					return -ENOSPC;
				}
				buf = read_blk(dir->i_dev, block);
				//rw_block(READ, dir->i_dev, block, &buf);
				if (!buf) {
					return -ENOSPC;
				}
			}
		}
		de = (struct ext2_dir *)(buf->b_data + inblock_offset);
		if (de->inode != 0 && strncmp(name, de->name, de->name_len) == 0) {
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
			//rw_block(WRITE, dir->i_dev, block, &buf);
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

i32 ext2_create(struct ext2_inode *dir, const i8 *name, i32 mode,
		struct ext2_inode **result) {
	i32 err;
	struct buffer *buf;
	struct ext2_inode *inode;
	struct ext2_dir *de;

	*result = NULL;
	if (!dir) {
		return -ENOENT;
	}
	inode = alloc_inode(dir->i_dev);
	if (!inode) {
		iput(dir);
		return -ENOSPC;
	}
	inode->i_mode = mode;
	inode->i_dirt = 1;
	if (!(err = ext2_add_entry(dir, name, &buf, &de))) {
		--inode->i_links_count;
		iput(inode);
		iput(dir);
		return err;
	}
	de->inode = inode->i_num;
	free(buf->b_data);
	free(buf);
	iput(dir);
	*result = inode;
	return 0;
}

struct buffer *ext2_find_entry(struct ext2_inode *dir, const i8 *name,
		struct ext2_dir **res_dir, struct ext2_dir **prev_dir) {
	struct buffer *buf;
	struct ext2_dir *de;
	u32 curr_off, block, inblock_offset;

	*res_dir = NULL;
	if (!dir) {
		return NULL;
	}
	if (strlen(name) > EXT2_NAME_LEN) {
		return NULL;
	}
	curr_off = inblock_offset = 0;
	if (!(block = dir->i_block[0])) {
		return NULL;
	}
	buf = read_blk(dir->i_dev, block);
	//rw_block(READ, dir->i_dev, block, &buf);
	if (!buf) {
		return NULL;
	}
	if (prev_dir) {
		*prev_dir = NULL;
	}
	while (curr_off < dir->i_size) {
		if (inblock_offset >= super_block.s_block_size) {
			block = ext2_bmap(dir, curr_off);	
			inblock_offset = 0;
			free(buf->b_data);
			free(buf);
			if (!block) {
				curr_off += BLOCK_SIZE;
				continue;
			}
			buf = read_blk(dir->i_dev, block);
			//rw_block(READ, dir->i_dev, block, &buf);
			if (!buf) {
				curr_off += BLOCK_SIZE;
				continue;
			}
		}
		de = (struct ext2_dir *)(buf->b_data + inblock_offset);
		if (strncmp(name, de->name, de->name_len) == 0) {
			*res_dir = de;
			return buf;
		}
		if (prev_dir) {
			*prev_dir = de;
		}
		inblock_offset += de->rec_len;
		curr_off += de->rec_len;
	}
	return NULL;
}

static i32 ext2_lookup(struct ext2_inode *dir, const i8 *name,
		struct ext2_inode **res) {
	struct buffer *buf;
	u32 inr;
	struct ext2_dir *de;

	*res = NULL;
	if (!dir) {
		return -ENOENT;
	}
	if (!check_permission(dir, MAY_EXEC)) {
		iput(dir);
		return -EACCES;
	}
	if (*name == '\0') {
		*res = dir;
		return 0;
	}
	if (!EXT2_S_ISDIR(dir->i_mode)) {
		iput(dir);
		return -ENOTDIR;
	}
	if (strlen(name) > EXT2_NAME_LEN) {
		return -ENAMETOOLONG;
	}
	if (!(buf = ext2_find_entry(dir, name, &de, NULL))) {
		iput(dir);
		return -ENOENT;
	}
	inr = de->inode;
	free(buf->b_data);
	free(buf);
	if (!(*res = iget(dir->i_dev, inr))) {
		iput(dir);
		return -EACCES;
	}
	iput(dir);
	return 0;
}

i32 dir_namei(const i8 *pathname, const i8 **name,
		struct ext2_inode **res_inode) {
	i32 error;
	const i8 *basename;
	struct ext2_inode *inode;
	i8 *pathname_dup, *saved_pathname, *tmp;

	saved_pathname = pathname_dup = strdup(pathname);
	if (pathname_dup[0] == '/') {
		inode = current_process->root;
		++pathname_dup;
	} else {
		inode = current_process->pwd;
	}
	++inode->i_count;

	while ((tmp = strsep(&pathname_dup, "/")) != NULL) {
		basename = tmp;
		if (!pathname_dup || *pathname_dup == '\0') {
			//free(saved_pathname);
			break;
		}
		++inode->i_count;
		error = ext2_lookup(inode, tmp, &inode);
		if (error) {
			iput(inode);
			free(saved_pathname);
			return error;
		}
	}
	*name = strdup(basename);
	*res_inode = inode;
	free(saved_pathname);
	return 0;
}

struct ext2_inode *namei(const i8 *pathname) {
	struct ext2_inode *dir, *inode;
	const i8 *basename;

	if (dir_namei(pathname, &basename, &dir)) {
		return NULL;
	}
	++dir->i_count; // lookup eats one 'i_count'
	if (ext2_lookup(dir, basename, &inode)) {
		iput(dir);
		return NULL;
	}
	iput(dir);
	inode->i_atime = get_current_time();
	inode->i_dirt = 1;
	return inode;
}

i32 open_namei(i8 *pathname, i32 oflags, i32 mode, struct ext2_inode **res_inode) {
	i32 error;
	const i8 *basename;
	struct ext2_inode *dir, *inode;

	if (dir_namei(pathname, &basename, &dir)) {
		return -ENOENT;
	}
	if (*basename == '\0') {
		if (!(oflags & (O_ACCMODE|O_CREAT|O_TRUNC))) {
			*res_inode = dir;
			return 0;
		}
		iput(dir);
		return -EISDIR;
	}
	++dir->i_count; // lookup eats one 'i_count'
	error = ext2_lookup(dir, basename, &inode);
	if (error) {
		if (!(oflags & O_CREAT)) {
			iput(dir);
			return error;
		}
		if (!check_permission(dir, MAY_WRITE)) {
			iput(dir);
			return -EACCES;
		}
		return ext2_create(dir, basename, mode, res_inode);
	}
	if ((oflags & O_EXCL) && (oflags & O_CREAT)) {
		iput(dir);
		iput(inode);
		return -EEXIST;
	}
	if (EXT2_S_ISDIR(inode->i_mode) && (oflags & (O_WRONLY | O_RDWR))) {
		return -EISDIR;
	}
	iput(dir);
	inode->i_atime = get_current_time();
	if (EXT2_S_ISREG(inode->i_mode) && oflags & O_TRUNC) {
		if (oflags & (O_WRONLY | O_RDWR)) {
			ext2_truncate(inode);
		} else {
			return -EACCES;
		}
	}
	*res_inode = inode;
	return 0;
}

static i32 subdir(struct ext2_inode *new, struct ext2_inode *old) {
	i32 inr, result;

	++new->i_count;
	result = 0;
	for (;;) {
		if (new = old) {
			result = 1;
			break;
		}
		if (new->i_dev != old->i_dev) {
			break;
		}
		inr = new->i_num;
		if (ext2_lookup(new, "..", &new)) {
			break;
		}
		if (new->i_num == inr) {
			break;
		}
	}
	iput(new);
	return result;
}

i32 ext2_rename(struct ext2_inode *old_dir, const i8 *old_name,
		struct ext2_inode *new_dir, const i8 *new_name) {
	i32 retval;
	struct buffer *old_buf, *new_buf, *dir_buf;
	struct ext2_inode *old_inode, *new_inode;
	struct ext2_dir *old_de, *pde, *new_de;

	old_buf = new_buf = NULL;
	old_inode = new_inode = NULL;
	retval = -ENOENT;

	old_buf = ext2_find_entry(old_dir, old_name, &old_de, &pde);
	if (!old_buf) {
		goto end_rename;
	}
	old_inode = iget(old_dir->i_dev, old_de->inode);
	if (!old_inode) {
		goto end_rename;
	}
	if (current_process->euid != old_inode->i_uid && 
			current_process->euid != old_dir->i_uid 
			&& current_process->euid != 0) {
		retval = -EPERM;
		goto end_rename;
	}
	new_buf = ext2_find_entry(new_dir, new_name, &new_de, NULL);
	if (new_buf) {
		new_inode = iget(new_dir->i_dev, new_de->inode);
		if (!new_inode) {
			free(new_buf);
			new_buf = NULL;
		}
	}
	if (new_inode == old_inode) {
		retval = 0;
		goto end_rename;
	}
	if (new_inode && EXT2_S_ISDIR(new_inode->i_mode)) {
		retval = -EEXIST;
		goto end_rename;
	}
	if (new_inode && current_process->euid != new_inode->i_uid && 
			current_process->euid != new_dir->i_uid 
			&& current_process->euid != 0) {
		retval = -EPERM;
		goto end_rename;
	}
	if (EXT2_S_ISDIR(old_inode->i_mode)) {
		retval = -EEXIST;
		if (new_buf) {
			goto end_rename;
		}
		retval = -EACCES;
		if (!check_permission(old_inode, MAY_WRITE)) {
			goto end_rename;
		}
		retval = -EINVAL;
		if (subdir(new_dir, old_inode)) {
			goto end_rename;
		}
		retval = -EIO;
		dir_buf = read_blk(old_inode->i_dev, old_inode->i_block[0]);
		//rw_block(READ, old_inode->i_dev, old_inode->i_block[0], &dir_buf);
		if (!dir_buf) {
			goto end_rename;
		}
		struct ext2_dir *fst_de = (struct ext2_dir *)dir_buf->b_data;
		i32 parent_i_num = ((struct ext2_dir *)(dir_buf + (fst_de->rec_len)))->inode;
		if (parent_i_num == old_dir->i_num) {
			goto end_rename;
		}
	}
	if (!new_buf) {
		ext2_add_entry(new_dir, new_name, &new_buf, &new_de);
	}
	retval = -ENOSPC;
	if (!new_buf) {
		goto end_rename;
	}
	new_de->inode = old_inode->i_num;
	if (((i8 *)new_de) + new_de->rec_len == ((char *)old_de)) {
		new_de->rec_len += old_de->rec_len;
	} else if (pde) {
		pde->rec_len += old_de->rec_len;
	} else {
		old_de->inode = 0;
	}
	if (new_inode) {
		--new_inode->i_links_count;
		new_inode->i_dirt = 1;
	}
	write_blk(old_buf);
	write_blk(new_buf);
	if (new_buf) {
		struct ext2_dir *fst_de = (struct ext2_dir *)dir_buf->b_data;
		((struct ext2_dir *)(dir_buf + (fst_de->rec_len)))->inode = new_dir->i_num;
		write_blk(dir_buf);
		--old_dir->i_links_count;
		++new_dir->i_links_count;
		write_blk(old_buf);
		write_blk(new_buf);
	}
	retval = 0;

end_rename:
	if (old_buf) {
		free(old_buf->b_data);
	}
	free(old_buf);
	if (new_buf) {
		free(new_buf->b_data);
	}
	free(new_buf);
	iput(old_inode);
	iput(new_inode);
	iput(old_dir);
	iput(new_dir);
	return retval;
}
