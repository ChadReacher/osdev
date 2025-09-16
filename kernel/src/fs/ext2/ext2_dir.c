#include <ext2.h>
#include <vfs.h>
#include <errno.h>
#include <heap.h>
#include <blk_dev.h>
#include <timer.h>
#include <string.h>
#include <panic.h>
#include <process.h> 
#include <bcache.h>

struct file_ops ext2_dir_ops = {
	NULL,
	NULL,
	NULL,
	ext2_readdir,
};

static i32 ext2_delete_entry(struct ext2_dir *dir, struct buffer *old_buf);

i32 ext2_unlink(struct vfs_inode *dir, const char *basename) {
	struct buffer *buf;
	struct vfs_inode *inode;
	struct ext2_dir *de;

	buf = ext2_find_entry(dir, basename, &de, NULL);
	if (!buf) {
		vfs_iput(dir);
		return -ENOENT;
	}
	inode = vfs_iget(dir->i_dev, de->inode);
	if (!inode) {
		vfs_iput(dir);
		brelse(buf);
		return -ENOENT;
	}
    if (inode->i_count > 1) {
        vfs_iput(dir);
        brelse(buf);
        vfs_iput(inode);
        return -EBUSY;
    }
	if (!EXT2_S_ISREG(inode->i_mode)) {
		vfs_iput(dir);
		vfs_iput(inode);
		brelse(buf);
		return -EPERM;
	}
	if (!inode->i_links_count) {
		inode->i_links_count = 1;
	}
	de->inode = 0;
	bwrite(buf);
	--inode->i_links_count;
	inode->i_dirt = 1;
	inode->i_ctime = get_current_time();

	vfs_iput(inode);
	vfs_iput(dir);
	brelse(buf);
	return 0;
}

i32 ext2_link(struct vfs_inode *dir, const i8 *basename, struct vfs_inode *inode) {
	i32 err;
	struct buffer *buf;
	struct ext2_dir *de;

	buf = ext2_find_entry(dir, basename, &de, NULL);
	if (buf) {
		brelse(buf);
		vfs_iput(dir);
		vfs_iput(inode);
		return -EEXIST;
	}
	err = ext2_add_entry(dir, basename, &buf, &de);
	if (err) {
		vfs_iput(dir);
		vfs_iput(inode);
		return err;
	}
	de->inode = inode->i_num;
	bwrite(buf);
	brelse(buf);
	vfs_iput(dir);
	++inode->i_links_count;
	inode->i_ctime = get_current_time();
	inode->i_dirt = 1;
	vfs_iput(inode);
    return 0;
}

static i32 subdir(struct vfs_inode *new, struct vfs_inode *old) {
	u32 inr, result;

	++new->i_count;
	result = 0;
	for (;;) {
		if (new == old) {
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
	vfs_iput(new);
	return result;
}

i32 ext2_rename(struct vfs_inode *old_dir, const i8 *old_name,
		struct vfs_inode *new_dir, const i8 *new_name) {
	i32 retval;
	struct buffer *old_buf, *new_buf, *dir_buf;
	struct vfs_inode *old_inode, *new_inode;
	struct ext2_dir *old_de, *new_de;

	dir_buf = old_buf = new_buf = NULL;
	old_inode = new_inode = NULL;
	retval = -ENOENT;

	old_buf = ext2_find_entry(old_dir, old_name, &old_de, NULL);
	if (!old_buf) {
		goto end_rename;
	}
	old_inode = vfs_iget(old_dir->i_dev, old_de->inode);
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
		new_inode = vfs_iget(new_dir->i_dev, new_de->inode);
		if (!new_inode) {
			brelse(new_buf);
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
		u32 parent_i_num;
		struct ext2_dir *fst_de;
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
		dir_buf = bread(old_inode->i_dev, old_inode->u.i_ext2.i_block[0]);
		if (!dir_buf) {
			goto end_rename;
		}
		fst_de = (struct ext2_dir *)dir_buf->data;
		parent_i_num = ((struct ext2_dir *)(dir_buf->data + (fst_de->rec_len)))->inode;
		if (parent_i_num != old_dir->i_num) {
			goto end_rename;
		}
		brelse(dir_buf);
	}
	if (!new_buf) {
		ext2_add_entry(new_dir, new_name, &new_buf, &new_de);
	}
	retval = -ENOSPC;
	if (!new_buf) {
		goto end_rename;
	}
	new_de->inode = old_inode->i_num;
	bwrite(new_buf);
	brelse(old_buf);
	old_buf = ext2_find_entry(old_dir, old_name, &old_de, NULL);
	if (!old_buf) {
		retval = -ENOENT;
		goto end_rename;
	}
	retval = ext2_delete_entry(old_de, old_buf);
	if (retval) {
		goto end_rename;
	}
	bwrite(old_buf);
	if (new_inode) {
		--new_inode->i_links_count;
		new_inode->i_dirt = 1;
	}
	if (EXT2_S_ISDIR(old_inode->i_mode)) {
		struct ext2_dir *fst_de, *snd_de;
		retval = -EIO;
		dir_buf = bread(old_inode->i_dev, old_inode->u.i_ext2.i_block[0]);
		if (!dir_buf) {
			goto end_rename;
		}
		fst_de = (struct ext2_dir *)dir_buf->data;
		snd_de = (struct ext2_dir *)(dir_buf->data + fst_de->rec_len);
		snd_de->inode = new_dir->i_num;
		bwrite(dir_buf);
		--old_dir->i_links_count;
		++new_dir->i_links_count;
		old_dir->i_dirt = 1;
		new_dir->i_dirt = 1;
	}
	retval = 0;

end_rename:
	brelse(old_buf);
	brelse(new_buf);
	brelse(dir_buf);
	vfs_iput(old_inode);
	vfs_iput(new_inode);
	vfs_iput(old_dir);
	vfs_iput(new_dir);
	return retval;
}

static i32 is_empty_dir(struct vfs_inode *inode) {
	u32 block;
	u32 inblock_offset, curr_off;
	struct buffer *buf;
	struct ext2_dir *de, *de1;

	buf = bread(inode->i_dev, inode->u.i_ext2.i_block[0]);
	if (!buf) { 
		return 1;
	}
	de = (struct ext2_dir *)buf->data;
	de1 = (struct ext2_dir *)(buf->data + de->rec_len);
	if (de->inode != inode->i_num || !de1->inode ||
			strcmp(".", de->name) || strcmp("..", de1->name)) {
		return 1;
	}
	curr_off = inblock_offset = de->rec_len + de1->rec_len;
	while (curr_off < (u32)inode->i_size) {
		if (inblock_offset >= inode->i_sb->s_block_size) {
			brelse(buf);
			inblock_offset = 0;
			block = ext2_bmap(inode, curr_off);
			if (!block) {
				curr_off += 1024;
				continue;
			}
			buf = bread(inode->i_dev, block);
			if (!buf) {
				curr_off += 1024;
				continue;
			}
		}
		de = (struct ext2_dir *)(buf->data + inblock_offset);
		if (de->inode) {
			brelse(buf);
			return 0;
		}
		inblock_offset += de->rec_len;
		curr_off += de->rec_len;
	}
	brelse(buf);
	return 1;
}

i32 ext2_rmdir(struct vfs_inode *dir, const char *basename) {
	i32 retval;
	struct buffer *buf;
	struct vfs_inode *inode;
	struct ext2_dir *de;

	inode = NULL;
	buf = ext2_find_entry(dir, basename, &de, NULL);
	if (!buf) {
		retval = -ENOENT;
		goto end;
	}
	if (!(inode = vfs_iget(dir->i_dev, de->inode))) {
		retval = -ENOENT;
		goto end;
	}
	if (inode->i_dev != dir->i_dev) {
		retval = -EXDEV;
		goto end;
	}
	if (inode == dir) {
		retval = -EPERM;
		goto end;
	}
	if (!EXT2_S_ISDIR(inode->i_mode)) {
		retval = -ENOTDIR;
		goto end;
	}
	if (!is_empty_dir(inode)) {
		retval = -ENOTEMPTY;
		goto end;
	}
	if (inode->i_count > 1) {
		retval = -EBUSY;
		goto end;
	}
	if (inode->i_links_count != 2) {
		debug("inode has links_count > 2, %d\r\n", inode->i_links_count);
	}
	de->inode = 0;
	bwrite(buf);
	inode->i_links_count = 0;
	inode->i_dirt = 1;
	--dir->i_links_count;
	dir->i_ctime = dir->i_mtime = get_current_time();
	dir->i_dirt = 1;
	retval = 0;
end:
	vfs_iput(dir);
	vfs_iput(inode);
	brelse(buf);
	return retval;
}

i32 ext2_mkdir(struct vfs_inode *dir, const char *basename, i32 mode) {
	i32 err;
	struct buffer *buf, *dir_block;
	struct vfs_inode *inode;
	struct ext2_dir *de;

	buf = ext2_find_entry(dir, basename, &de, NULL);
	if (buf) {
		brelse(buf);
		vfs_iput(dir);
		return -EEXIST;
	}
	inode = ext2_alloc_inode(dir->i_dev);
	if (!inode) {
		vfs_iput(dir);
		return -ENOSPC;
	}
	inode->i_dirt = 1;
	inode->i_mtime = inode->i_atime = get_current_time();
	if (!(inode->u.i_ext2.i_block[0] = ext2_alloc_block(inode->i_dev))) {
		vfs_iput(dir);
		--inode->i_links_count;
		vfs_iput(inode);
		return -ENOSPC;
	}
	inode->i_size = 1024;
	inode->i_blocks += 2;
	if (!(dir_block = bread(inode->i_dev, inode->u.i_ext2.i_block[0]))) {
		inode->i_blocks -= 2;
		vfs_iput(dir);
		--inode->i_links_count;
		vfs_iput(inode);
		return -EIO;
	}
	de = (struct ext2_dir *)(dir_block->data);
	de->inode = inode->i_num;
	de->rec_len = 12;
	de->name_len = 1;
	memcpy(de->name, ".", 1);
	de = (struct ext2_dir *)(dir_block->data + de->rec_len);
	de->inode = dir->i_num;
	de->rec_len = 1024 - 12;
	de->name_len = 2;
	memcpy(de->name, "..", 2);
	inode->i_links_count = 2;
	bwrite(dir_block);
	brelse(dir_block);
	inode->i_mode = EXT2_S_IFDIR | (mode & 0777 & ~current_process->umask);
	inode->i_dirt = 1;
	err = ext2_add_entry(dir, basename, &buf, &de);
	if (err) {
		inode->i_blocks -= 2;
		vfs_iput(dir);
		inode->i_links_count = 0;
		vfs_iput(inode);
		return -ENOSPC;
	}
	de->inode = inode->i_num;
	bwrite(buf);
	++dir->i_links_count;
	dir->i_dirt = 1;

	brelse(buf);
	vfs_iput(dir);
	vfs_iput(inode);
	return 0;
}

i32 ext2_symlink(struct vfs_inode *dir, const i8 *name, const i8*symname) {
	i32 err;
    struct vfs_inode *inode;
	struct buffer *buf;
	struct ext2_dir *de;

	inode = ext2_alloc_inode(dir->i_dev);
	if (!inode) {
		vfs_iput(dir);
		return -ENOSPC;
	}
	inode->i_dirt = 1;
	inode->i_mode = EXT2_S_IFLNK | 0777;
	inode->i_mtime = inode->i_atime = get_current_time();
	extern struct vfs_inode_ops ext2_inode_symlink_ops;
	inode->i_ops = &ext2_inode_symlink_ops;
	if (!(inode->u.i_ext2.i_block[0] = ext2_alloc_block(inode->i_dev))) {
		vfs_iput(dir);
		--inode->i_links_count;
        inode->i_dirt = 1;
		vfs_iput(inode);
		return -ENOSPC;
	}
    buf = bread(inode->i_dev, inode->u.i_ext2.i_block[0]);
    if (!buf) {
		vfs_iput(dir);
		--inode->i_links_count;
        inode->i_dirt = 1;
		vfs_iput(inode);
		return -EIO;
    }
    u32 i = 0;
    for (i = 0; i < strlen(symname); ++i) {
        buf->data[i] = symname[i];
    }
    buf->data[i] = '\0';
    bwrite(buf);
	brelse(buf);

	inode->i_size = i;
	inode->i_dirt = 1;

	buf = ext2_find_entry(dir, name, &de, NULL);
	if (buf) {
		brelse(buf);
		vfs_iput(dir);
		--inode->i_links_count;
        inode->i_dirt = 1;
		vfs_iput(inode);
		return -EEXIST;
	}
	err = ext2_add_entry(dir, name, &buf, &de);
	if (err) {
		brelse(buf);
		vfs_iput(dir);
		--inode->i_links_count;
        inode->i_dirt = 1;
		vfs_iput(inode);
		return err;
	}
	de->inode = inode->i_num;
	bwrite(buf);
	brelse(buf);
    vfs_iput(inode);
    vfs_iput(dir);
    return 0;
}

i32 ext2_readlink(struct vfs_inode *inode, i8 *buf, i32 bufsiz) {
	struct buffer *bbuf;

	if (!EXT2_S_ISLNK(inode->i_mode)) {
		return -EINVAL;
	}
    bbuf = bread(inode->i_dev, inode->u.i_ext2.i_block[0]);
	vfs_iput(inode);
	if (!bbuf) {
		return 0;
	}
	i32 i;
	for (i = 0; i < bufsiz; ++i) {
		buf[i] = bbuf->data[i];
	}

	brelse(bbuf);
	return i;
}

struct vfs_inode *ext2_follow_link(struct vfs_inode *inode, struct vfs_inode *base) {
	i8 *name;
	struct buffer *buf;
	if (!base) {
		base = current_process->root;
		++base->i_count;
	}
	if (!inode) {
		vfs_iput(base);
		return NULL;
	}	
	if (!S_ISLNK(inode->i_mode)) {
		vfs_iput(base);
		return inode;
	}
	if (current_process->symlink_count > MAX_LINK_COUNT) {
		vfs_iput(inode);
		vfs_iput(base);
		return NULL; 
	}
	if (!inode->u.i_ext2.i_block[0]) {
		vfs_iput(inode);
		vfs_iput(base);
		return NULL;
	}
	if (!(buf = bread(inode->i_dev, inode->u.i_ext2.i_block[0]))) {
		vfs_iput(inode);
		vfs_iput(base);
		return NULL;
	}
	vfs_iput(inode);
	name = buf->data;
	++current_process->symlink_count;
	if (vfs_namei(name, base, 1, &inode) != 0) {
		inode = NULL;
	}
	--current_process->symlink_count;
	brelse(buf);
	return inode;
}

static i32 ext2_delete_entry(struct ext2_dir *dir, struct buffer *old_buf) {
	struct ext2_dir *de, *pde;
	i32 curr_off = 0;
	pde = NULL;
	de = (struct ext2_dir *)(old_buf->data);
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
		de = (struct ext2_dir *)(old_buf->data + curr_off);
	}
	return -ENOENT;
}

i32 ext2_mount(struct vfs_inode *dir, u32 dev, const char *basename) {
	struct ext2_dir *de = NULL;

	struct buffer *buf = ext2_find_entry(dir, basename, &de, NULL);
	if (!buf) {
		vfs_iput(dir);
		return -ENOENT;
	}

	struct vfs_inode *inode = vfs_iget(dir->i_dev, de->inode);
	if (!inode) {
		vfs_iput(dir);
		brelse(buf);
		return -ENOENT;
	}

    i32 err = vfs_do_mount(dev, inode);
    if (err) {
        vfs_iput(dir);
        brelse(buf);
        vfs_iput(inode);
        return err;
    }

	vfs_iput(dir);
    brelse(buf);
    vfs_iput(inode);
    return 0;
}
