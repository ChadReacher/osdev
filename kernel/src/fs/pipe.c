#include <pipe.h>
#include <signal.h>
#include <process.h>
#include <scheduler.h>
#include <errno.h>
#include <heap.h>

static i32 bad_pipe_write(struct vfs_inode *inode, struct file *fp, i8 *buf, i32 count);
static i32 bad_pipe_read(struct vfs_inode *inode, struct file *fp, i8 *buf, i32 count);
static i32 pipe_read(struct vfs_inode *inode, struct file *fp, i8 *buf, i32 count);
static i32 pipe_write(struct vfs_inode *inode, struct file *fp, i8 *buf, i32 count);

struct file_ops pipe_read_ops = {
	pipe_read,
    bad_pipe_write,
	NULL
};

struct file_ops pipe_write_ops = {
	bad_pipe_read,
    pipe_write,
	NULL
};

static i32 bad_pipe_write(struct vfs_inode *inode, struct file *fp, i8 *buf, i32 count) {
	(void)inode;
	(void)fp;
	(void)buf;
	(void)count;
	return -EBADF;
}

static i32 bad_pipe_read(struct vfs_inode *inode, struct file *fp, i8 *buf, i32 count) {
	(void)inode;
	(void)fp;
	(void)buf;
	(void)count;
	return -EBADF;
}

static i32 pipe_read(struct vfs_inode *inode, struct file *fp, i8 *buf, i32 count) {
	(void)fp;
	i8 *b = buf;
	
	while (PIPE_EMPTY(inode)) {
		wake_up(&inode->i_wait);
		// there are no writers
		if (inode->i_count != 2) {
			return 0;
		}
		goto_sleep(&inode->i_wait);
	}
	while (count > 0 && !(PIPE_EMPTY(inode))) {
		--count;
		*b = (inode->u.i_pipe.i_buf)[PIPE_TAIL(inode)];
		++b;
		++PIPE_TAIL(inode);
		PIPE_TAIL(inode) &= (4096 - 1);
	}
	return b - buf;
}

static i32 pipe_write(struct vfs_inode *inode, struct file *fp, i8 *buf, i32 count) {
	(void)fp;
	i8 *b = buf;

	wake_up(&inode->i_wait);
	// there are no readers
	if (inode->i_count != 2) {
		sigaddset(&current_process->sigpending, SIGPIPE);
		return -1;
	}
	while (count-- > 0) {
		while (PIPE_FULL(inode)) {
			wake_up(&inode->i_wait);
			if (inode->i_count != 2) {
				sigaddset(&current_process->sigpending, SIGPIPE);
				return -1;
			}
			goto_sleep(&inode->i_wait);
		}
		(inode->u.i_pipe.i_buf)[PIPE_HEAD(inode)] = *b;
		++b;
		++PIPE_HEAD(inode);
		PIPE_HEAD(inode) &= (4096 - 1);
		wake_up(&inode->i_wait);
	}
	wake_up(&inode->i_wait);
	return b - buf;
}


struct vfs_inode *get_pipe_inode() {
	struct vfs_inode *inode;

	if (!(inode = get_empty_inode())) {
		return NULL;
	}
	inode->i_count = 2;
	inode->i_pipe = 1;
	inode->u.i_pipe.i_buf = malloc(4096);
	inode->u.i_pipe.i_head = 0;
	inode->u.i_pipe.i_tail = 0;
	return inode;
}
