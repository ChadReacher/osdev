#include <syscall.h>
#include <stdio.h>
#include <panic.h>
#include <fd.h>
#include <vfs.h>
#include <debug.h>
#include <fcntl.h>
#include <string.h>
#include <keyboard.h>
#include <screen.h>
#include <process.h>
#include <elf.h>
#include <paging.h>
#include <scheduler.h>

extern u32 next_pid;
extern process_t *current_process;

syscall_handler_t syscall_handlers[NB_SYSCALLS];

void syscall_init() {
	syscall_register_handler(SYSCALL_TEST, syscall_test);
	syscall_register_handler(SYSCALL_OPEN, syscall_open);
	syscall_register_handler(SYSCALL_CLOSE, syscall_close);
	syscall_register_handler(SYSCALL_READ, syscall_read);
	syscall_register_handler(SYSCALL_WRITE, syscall_write);
	syscall_register_handler(SYSCALL_LSEEK, syscall_lseek);
	syscall_register_handler(SYSCALL_UNLINK, syscall_unlink);
	syscall_register_handler(SYSCALL_YIELD, syscall_yield);
	syscall_register_handler(SYSCALL_EXEC, syscall_exec);
	syscall_register_handler(SYSCALL_FORK, syscall_fork);
	syscall_register_handler(SYSCALL_EXIT, syscall_exit);
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
	} else if((vfs_node->flags & O_TRUNC) == O_TRUNC) {
		vfs_trunc(vfs_node);
	}

	i32 fd = proc_get_fd(current_process);
	if (fd == -1) {
		PANIC("%s", "We have run out of file descriptors\r\n");
	}
	current_process->fds[fd] = (file) {
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
	file *f = &current_process->fds[fd];
	if (!f->used || !f->vfs_node) {
		DEBUG("Bad descriptor - %d\r\n", fd);
		regs->eax = 1;
		return;
	}
	vfs_close(f->vfs_node);
	memset(&current_process->fds[fd], 0, sizeof(file));
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

	file *f = &current_process->fds[fd];
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

	file *f = &current_process->fds[fd];

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

	file *f = &current_process->fds[fd];

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

void syscall_yield(registers_state *regs) {
	schedule(regs);
}

void syscall_exec(registers_state *regs) {
	/*
	i8 *pathname = (i8 *)regs->ebx;

	vfs_node_t *vfs_node = vfs_get_node(pathname);
	if (!vfs_node) {
		regs->eax = -1;
		return;
	}
	u32 *data = malloc(vfs_node->length);
	memset((i8 *)data, 0, vfs_node->length);
	vfs_read(vfs_node, 0, vfs_node->length, (i8 *)data);

	elf_header_t *elf = (elf_header_t *)data;

	if (is_elf(elf) != ET_EXEC) {
		regs->eax = -1;
		return;
	}

	elf_program_header_t* program_header = (elf_program_header_t*)((u32)data + elf->phoff);

	for (u32 i = 0; i < elf->ph_num; ++i) {
		if (program_header[i].type == PT_LOAD) {
			u32 filesz = program_header->filesz; // Size in file
			u32 vaddr = program_header->vaddr;
			u32 offset = program_header->offset; // Offset in file

			if (filesz == 0) {
				regs->eax = -1;
				return;
			}

			u32 len_in_blocks = filesz / 4096;
			if (len_in_blocks % 4096 != 0 || len_in_blocks == 0) {
				++len_in_blocks;
			}

			void *code = (void *)((u32)data + offset);
			for (u32 i = 0, addr = 0x0; i < len_in_blocks; ++i, addr += 0x1000) {
				if (!virtual_to_physical(addr)) {
					// If the page is not mapped
					void *new_code_page = allocate_blocks(1);
					map_page((void *)new_code_page, (void *)addr);
					memcpy((void *)addr, code + addr, 0x1000);
				} else {
					memcpy((void *)addr, code + addr, 0x1000);
				}
			}
		}
	}

	void *kernel_stack = malloc(4096);
	memset(kernel_stack, 0, 4096);
	current_process->kernel_stack_bottom = kernel_stack;
	current_process->kernel_stack_top = (u8 *)kernel_stack + 4096 - 1;
	current_process->regs.eip = 0;
	current_process->regs.cs = 0x1B;
	current_process->regs.ds = 0x23;
	current_process->regs.useresp = 0xBFFFFFFB;
	current_process->regs.ebp = 0xBFFFFFFB;

	memset(0xBFFFF000, 0, 4092);

	tss_set_stack(current_process->kernel_stack_bottom);

	u32 proc_eax = current_process->regs.eax;
	u32 proc_ecx = current_process->regs.ecx;
	u32 proc_edx = current_process->regs.edx;
	u32 proc_ebx = current_process->regs.ebx;
	u32 proc_esp = current_process->regs.useresp;
	u32 proc_ebp = current_process->regs.ebp;
	u32 proc_esi = current_process->regs.esi;
	u32 proc_edi = current_process->regs.edi;
	u32 proc_eip = current_process->regs.eip;

	extern void context_switch(u32 eax, u32 ecx, u32 edx, u32 ebx, u32 useresp, u32 ebp, u32 esi, u32 edi, u32 eip);

	context_switch(proc_eax, proc_ecx, proc_edx, proc_ebx, proc_esp, proc_ebp, proc_esi, proc_edi, proc_eip);
*/
}

void syscall_fork(registers_state *regs) {
	process_t *process = proc_alloc();

	process->directory = paging_copy_page_dir(true);
	// TODO: clone file descriptors
	process->cwd = strdup("/");

	*process->regs = *current_process->regs;

	process->regs->eax = 0; 
	current_process->regs->eax = process->pid; 
}

void syscall_exit(registers_state *regs) {
	//process_kill(current_process);
}
