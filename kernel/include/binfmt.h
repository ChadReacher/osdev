#pragma once

#include "types.h"
#include "isr.h"
#include "vfs.h"

struct binparams {
	struct vfs_inode *inode;
	u32 argc;
	i8 **argv;
	u32 envc;
	i8 **envp;
	i8 buffer[128];
};

i32 read_exec_buffer(struct vfs_inode *inode, i8 *buf, i32 bufsz, i32 offset);

i32 search_binary_handler(struct binparams *bprm, struct registers_state *regs);
