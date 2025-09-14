#include <pipe.h>
#include <signal.h>
#include <process.h>
#include <scheduler.h>
#include <errno.h>
#include <heap.h>
#include <panic.h>

static void pipe_wake(struct vfs_inode *inode);
static void pipe_sleep(struct vfs_inode *inode);
static i32 bad_pipe_write(struct vfs_inode *inode, struct file *fp, i8 *buf, i32 count);
static i32 bad_pipe_read(struct vfs_inode *inode, struct file *fp, i8 *buf, i32 count);
static i32 pipe_read(struct vfs_inode *inode, struct file *fp, i8 *buf, i32 count);
static i32 pipe_write(struct vfs_inode *inode, struct file *fp, i8 *buf, i32 count);

static void pipe_wake(struct vfs_inode *inode) {
    process_wakeup(inode->i_wait);
    inode->i_wait = NULL;
}

static void pipe_sleep(struct vfs_inode *inode) {
    inode->i_wait = current_process;
    process_sleep();
}

struct file_ops pipe_read_ops = {
    NULL,
    pipe_read,
    bad_pipe_write,
    NULL,
};

struct file_ops pipe_write_ops = {
    NULL,
    bad_pipe_read,
    pipe_write,
    NULL
};

static i32 bad_pipe_write(UNUSED struct vfs_inode *inode,
                          UNUSED struct file *fp, UNUSED i8 *buf,
                          UNUSED i32 count) {
    return -EBADF;
}

static i32 bad_pipe_read(UNUSED struct vfs_inode *inode, UNUSED struct file *fp,
                         UNUSED i8 *buf, UNUSED i32 count) {
    return -EBADF;
}

static i32 pipe_read(struct vfs_inode *inode, UNUSED struct file *fp, i8 *buf,
                     i32 count) {
    i8 *b = buf;

    while (PIPE_EMPTY(inode)) {
        pipe_wake(inode);
        // there are no writers
        if (inode->i_count != 2) {
            return 0;
        }
        pipe_sleep(inode);
    }
    while (count > 0 && !(PIPE_EMPTY(inode))) {
        --count;
        *b = (inode->u.i_pipe.i_buf)[PIPE_TAIL(inode)];
        ++b;
        ++PIPE_TAIL(inode);
        PIPE_TAIL(inode) &= (PAGE_SIZE - 1);
    }
    return b - buf;
}

static i32 pipe_write(struct vfs_inode *inode, UNUSED struct file *fp, i8 *buf,
                     i32 count) {
    i8 *b = buf;

    pipe_wake(inode);
    // there are no readers
    if (inode->i_count != 2) {
        sigaddset(&current_process->sigpending, SIGPIPE);
        return -1;
    }
    while (count-- > 0) {
        while (PIPE_FULL(inode)) {
            pipe_wake(inode);
            inode->i_wait = NULL;
            if (inode->i_count != 2) {
                sigaddset(&current_process->sigpending, SIGPIPE);
                return -1;
            }
            pipe_sleep(inode);
        }
        (inode->u.i_pipe.i_buf)[PIPE_HEAD(inode)] = *b;
        ++b;
        ++PIPE_HEAD(inode);
        PIPE_HEAD(inode) &= (PAGE_SIZE - 1);
        pipe_wake(inode);
    }
    pipe_wake(inode);
    return b - buf;
}


struct vfs_inode *pipe_get_inode(void) {
    struct vfs_inode *inode = NULL;

    if (!(inode = get_empty_inode())) {
        return NULL;
    }
    inode->i_count = 2;
    inode->i_pipe = 1;
    inode->u.i_pipe.i_buf = malloc(PAGE_SIZE);
    if (inode->u.i_pipe.i_buf == NULL) {
        debug("Failed to allocate enough memory for pipe buffer");
        vfs_iput(inode);
        return NULL;
    }
    inode->u.i_pipe.i_head = 0;
    inode->u.i_pipe.i_tail = 0;
    return inode;
}
