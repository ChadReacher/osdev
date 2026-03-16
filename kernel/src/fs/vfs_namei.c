#include <vfs.h>
#include <string.h>
#include <process.h>
#include <heap.h>
#include <timer.h>
#include <fcntl.h>
#include <errno.h>
#include <panic.h>

/* vfs_dirnamei - converts the path name to VFS inode
 * corresponding to the last directory in the path name
 *
 * res_basename - the name of the last directory in the path
 * res_inode    - the VFS inode of the last directory in the path
 */
// If `base` is empty, then the base inode is current working directory.
i32 vfs_dirnamei(const i8 *pathname, struct vfs_inode *base, const i8 **res_basename,
             struct vfs_inode **res_inode) {
    i32 error;
    const i8 *basename = NULL;
    struct vfs_inode *inode = NULL;
    i8 *pathname_dup, *saved_pathname;
    i8 *tmp = NULL;

    assert(pathname != NULL);

    saved_pathname = pathname_dup = strdup(pathname);
    if (pathname_dup[0] == '/') {
        vfs_iput(base);
        base = current_process->root;
        while (*pathname_dup == '/') {
            ++pathname_dup;
        }
        ++base->i_count;
    } else {
        if (!base) {
            base = current_process->pwd;
            ++base->i_count;
        }
    }

    while ((tmp = strsep(&pathname_dup, "/")) != NULL) {
        basename = tmp;
        if (!pathname_dup || *pathname_dup == '\0') {
            break;
        }
        if (!base->i_ops || !base->i_ops->lookup) {
            vfs_iput(base);
            free(saved_pathname);
            return -ENOENT; 
        }
        ++base->i_count; /* lookup eats one 'i_count' */
        error = vfs_lookup(base, tmp, &inode);
        if (error) {
            vfs_iput(base);
            free(saved_pathname);
            return error;
        }
        if (!inode->i_ops || !inode->i_ops->followlink) {
            vfs_iput(base);
            base = inode;
            continue;
        }
        struct vfs_inode *res = NULL;
        error = inode->i_ops->followlink(inode, base, &res);
        if (error) {
            vfs_iput(base);
            free(saved_pathname);
            return error;
        }
        base = res;
    }
    if (basename == NULL) {
        vfs_iput(base);
        free(saved_pathname);
        return -ENOENT;
    }
    *res_basename = strdup(basename);
    *res_inode = base;
    free(saved_pathname);
    return 0;
}

// Converts a `pathname` to the VFS inode using another
// VFS inode as `base` of search and follows links (if specified)
i32 vfs_namei(const i8 *pathname, struct vfs_inode *base, i32 follow_links, struct vfs_inode **res) {
    const i8 *basename = NULL;
    struct vfs_inode *dir = NULL;
    struct vfs_inode *inode = NULL;

    // vfs_dirnamei eats one 'i_count'
    if (base) {
        ++base->i_count;
    }
    i32 err = vfs_dirnamei(pathname, base, &basename, &dir);
    if (err != 0) {
        return err;
    }
    if (!dir->i_ops || !dir->i_ops->lookup) {
        vfs_iput(dir);
        return -ENOENT;
    }
    ++dir->i_count; /* lookup eats one 'i_count' */
    err = vfs_lookup(dir, basename, &inode);
    vfs_iput(dir);
    if (err != 0) {
        return err;
    }

    if (follow_links && inode->i_ops && inode->i_ops->followlink) {
        struct vfs_inode *res = NULL;
        err = inode->i_ops->followlink(inode, base, &res);
        if (err != 0) {
            return err;
        }
        inode = res;
    }

    inode->i_atime = get_current_time();
    inode->i_dirt = 1;
    *res = inode;

    return 0;
}

/* vfs_open_namei - namei but for open() syscall */
i32 vfs_open_namei(i8 *pathname, i32 oflags, i32 mode, struct vfs_inode **res_inode) {
    const i8 *basename;
    struct vfs_inode *dir = NULL;
    struct vfs_inode *inode = NULL;

    i32 err = vfs_dirnamei(pathname, NULL, &basename, &dir);
    if (err != 0) {
        return err;
    }
    // this happens if 'pathname' is '/'
    if (*basename == '\0') {
        if (!(oflags & (O_ACCMODE|O_CREAT|O_TRUNC))) {
            *res_inode = dir;
            return 0;
        }
        vfs_iput(dir);
        return -EISDIR;
    }

    if (!dir->i_ops || !dir->i_ops->lookup) {
        vfs_iput(dir);
        return -ENOENT;
    }
    ++dir->i_count; /* lookup eats one 'i_count' */
    err = vfs_lookup(dir, basename, &inode);

    if (err) {
        if (!(oflags & O_CREAT)) {
            vfs_iput(dir);
            return err;
        }
        if (!check_permission(dir, MAY_WRITE)) {
            vfs_iput(dir);
            return -EACCES;
        }

        if (!dir->i_ops || !dir->i_ops->create) {
            vfs_iput(dir);
            return -ENOENT;
        }
        return dir->i_ops->create(dir, basename, mode, res_inode);
    }

    if (inode->i_ops && inode->i_ops->followlink) {
        struct vfs_inode *res = NULL;
        err = inode->i_ops->followlink(inode, dir, &res);
        if (err) {
            vfs_iput(dir);
            return err;
        }
        inode = res;
    }


    if ((oflags & O_EXCL) && (oflags & O_CREAT)) {
        vfs_iput(dir);
        vfs_iput(inode);
        return -EEXIST;
    }

    if (S_ISDIR(inode->i_mode) && (oflags & (O_WRONLY | O_RDWR))) {
        vfs_iput(dir);
        vfs_iput(inode);
        return -EISDIR;
    }
    vfs_iput(dir);
    inode->i_atime = get_current_time();
    inode->i_dirt = 1;

    if (S_ISREG(inode->i_mode) && oflags & O_TRUNC) {
        if (oflags & (O_WRONLY | O_RDWR)) {
            if (!inode->i_ops || !inode->i_ops->truncate) {
                vfs_iput(inode);
                return -ENOENT;
            }
            inode->i_ops->truncate(inode, 0);
        } else {
            vfs_iput(inode);
            return -EACCES;
        }
    }
    *res_inode = inode;
    return 0;
}

extern struct vfs_superblock superblocks[NR_SUPERBLOCKS];

// Performs lookup in the @dir directory for the @name file and storing the resulting inode at @res
// It 'consumes' the @dir inode exactly once.
// In the case of mount point boundary it returns the host side of the mount point, not fast-forwarding. 
// For example:
// ```
// dir = "/" (it's mount point root, in rootfs it's '/mnt')
// vfs_lookup(dir, "..") will return host side '/mnt' _not_ the host '/' immediately.
// ```
i32 vfs_lookup(struct vfs_inode *dir, const i8 *name, struct vfs_inode **res) {
    struct vfs_inode *inode = NULL;
    i32 err = -1;
    assert(dir != NULL);
    assert(dir->i_count >= 2);
    assert(name != NULL);
    assert(res != NULL);

    for (int i = 0; i < NR_SUPERBLOCKS; ++i) {
        if (superblocks[i].s_dev == 0) {
            continue;
        }
        bool is_root_of_mount = superblocks[i].s_root == dir;
        if (strcmp(name, "..") == 0 && is_root_of_mount) {
            struct vfs_inode *host_side = superblocks[i].s_mounted;
            if (host_side != NULL) {
                vfs_iput(dir);
                ++host_side->i_count;
                *res = host_side;
                return 0;
            }
        }
    }

    assert(dir->i_ops != NULL);
    assert(dir->i_ops->lookup != NULL);
    err = dir->i_ops->lookup(dir, name, &inode);
    if (err < 0) {
        goto exit;
    }

    for (int i = 0; i < NR_SUPERBLOCKS; ++i) {
        if (superblocks[i].s_dev == 0) {
            continue;
        }
        bool is_mount_point = superblocks[i].s_mounted == inode;
        if (is_mount_point) {
            struct vfs_inode *mount_side = superblocks[i].s_root;
            ++mount_side->i_count;
            vfs_iput(inode);
            inode = mount_side;
            break;
        }
    }

    *res = inode;
    err = 0;
exit:
    return err;
}

i32 check_permission(struct vfs_inode *inode, i32 mask) {
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
