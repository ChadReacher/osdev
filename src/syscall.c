#include "syscall.h"
#include "stdio.h"
#include "panic.h"
#include "fd.h"
#include "vfs.h"
#include "debug.h"
#include "fcntl.h"
#include "string.h"
#include "keyboard.h"
#include "screen.h"

extern file fds[NB_DESCRIPTORS];

syscall_handler_t syscall_handlers[NB_SYSCALLS];

void syscall_init() {
	syscall_register_handler(SYSCALL_TEST, syscall_test);
	syscall_register_handler(SYSCALL_OPEN, syscall_open);
	syscall_register_handler(SYSCALL_CLOSE, syscall_close);
	syscall_register_handler(SYSCALL_READ, syscall_read);
	syscall_register_handler(SYSCALL_WRITE, syscall_write);
	syscall_register_handler(SYSCALL_LSEEK, syscall_lseek);
	syscall_register_handler(SYSCALL_UNLINK, syscall_unlink);
}

void syscall_register_handler(u8 id, syscall_handler_t handler) {
	syscall_handlers[id] = handler;
}

void syscall_handler(registers_state *regs) {
	syscall_handler_t handler = syscall_handlers[regs->eax];

	if (handler != 0) {
		handler(regs);
		return;
	}

	PANIC("Received unimplemented syscall: %d\n", regs->eax);
}


void syscall_test(registers_state *regs) {
	kprintf("Hello from syscall_test(), %s\n", regs->ebx);
}

void syscall_open(registers_state *regs) {
	i8 *filename = (i8* )regs->ebx;
	u32 oflags = regs->ecx;
	u32 mode = regs->edx;

	vfs_node_t *vfs_node;

	vfs_node = vfs_get_node(filename);
	if (!vfs_node) {
		if (oflags & O_CREAT) {
			vfs_create(filename, mode);
			vfs_node = vfs_get_node(filename);
		}
	} else if ((vfs_node->flags & FS_DIRECTORY) == FS_DIRECTORY) {
		DEBUG("Cannot open a directory - %s\r\n", filename);
		regs->eax = -1;
		return;
	}

	i32 fd = fd_get();
	if (fd == -1) {
		PANIC("%s", "We have run out of file descriptors\r\n");
	}
	fds[fd] = (file) {
		.vfs_node = vfs_node,
		.offset = 0,
		.flags = oflags,
		.used = true,
	};
	vfs_open(vfs_node, oflags);
	regs->eax = fd;
}

void syscall_close(registers_state *regs) {
	u32 fd = regs->ebx;
	if (fd < 3 || fd >= NB_DESCRIPTORS) {
		DEBUG("Invalid file descriptor - %d\r\n", fd);
		regs->eax = 1;
		return;
	}
	file *f = &fds[fd];
	if (!f->used || !f->vfs_node) {
		DEBUG("Bad descriptor - %d\r\n", fd);
		regs->eax = 1;
		return;
	}
	vfs_close(f->vfs_node);
	memset(&fds[fd], 0, sizeof(file));
	regs->eax = 0;
}

void syscall_read(registers_state *regs) {
	i32 fd = regs->ebx;
	i8 *buf = (i8 *)regs->ecx;
	u32 count = regs->edx;

	if (fd == FD_STDIN) {
		u8 scancode = keyboard_get_scancode();
		if (scancode) {
			buf[0] = scancode;
		}
		regs->eax = 1;
		return;
	}

	if (fd < 3 || fd >= NB_DESCRIPTORS) {
		DEBUG("Invalid file descriptor - %d\r\n", fd);
		regs->eax = 0;
		return;
	}

	file *f = &fds[fd];
	if (!f->used || !f->vfs_node) {
		DEBUG("Bad descriptor - %d\r\n", fd);
		regs->eax = 0;
		return;
	}
	if (f->flags & O_WRONLY) {
		DEBUG("%s", "Cannot read file with O_WRONLY flag\r\n");
		regs->eax = 0;
		return;
	}
	u32 have_read = vfs_read(f->vfs_node, f->offset, count, buf);
	f->offset += have_read;

	regs->eax = have_read;
}

void syscall_write(registers_state *regs) {
	i32 fd = regs->ebx;
	i8 *buf = (i8 *)regs->ecx;
	u32 count = regs->edx;

	if (fd == FD_STDOUT || fd == FD_STDERR) {
		for (u32 i = 0; i < count; ++i) {
			screen_print_char(buf[i]);
		}
		return;
	}

	if (fd < 3 || fd >= NB_DESCRIPTORS) {
		DEBUG("Invalid file descriptor - %d\r\n", fd);
		regs->eax = 0;
		return;
	}

	file *f = &fds[fd];

	if (!f->used || !f->vfs_node) {
		DEBUG("Bad descriptor - %d\r\n", fd);
		regs->eax = 0;
		return;
	}
	if (!(f->flags & (O_WRONLY | O_RDWR))) {
		DEBUG("%s", "File is not opened for writing.\r\n");
		regs->eax = 0;
		return;
	}
	if ((f->flags & O_APPEND) == O_APPEND) {
		f->offset = f->vfs_node->length;
	}
	u32 have_written = vfs_write(f->vfs_node, f->offset, count, buf);
	f->offset += have_written;
	regs->eax = have_written;
}

void syscall_lseek(registers_state *regs) {
	i32 fd = regs->ebx;
	i32 offset = regs->ecx;
	i32 whence = regs->edx;

	if (fd < 3 || fd >= NB_DESCRIPTORS) {
		DEBUG("Invalid file descriptor - %d\r\n", fd);
		regs->eax = 0;
		return;
	}

	file *f = &fds[fd];

	if (!f->used || !f->vfs_node) {
		DEBUG("Bad descriptor - %d\r\n", fd);
		regs->eax = 0;
		return;
	}
	if (whence == SEEK_SET) {
		f->offset = offset;
	} else if (whence == SEEK_CUR) {
		f->offset += offset;
	} else if (whence == SEEK_END) {
		f->offset = f->vfs_node->length + offset;
	} else {
		DEBUG("Invalid whence argument - %d\r\n", whence);
		regs->eax = -1;
		return;
	}

	regs->eax = f->offset;
}

void syscall_unlink(registers_state *regs) {
	i8 *filename = (i8 *)regs->ebx;
	i32 ret = vfs_unlink(filename);
	regs->eax = ret;
}
