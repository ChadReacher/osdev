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

extern file fds[NB_DESCRIPTORS];
extern process_t *current_process;
extern u32 pid_gen;

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

void syscall_yield(registers_state *regs) {
	switch_process(regs);
}

void syscall_exec(registers_state *regs) {
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
	current_process->kernel_stack = (u8 *)kernel_stack + 4096 - 1;
	current_process->regs.eip = 0;
	current_process->regs.cs = 0x1B;
	current_process->regs.ds = 0x23;
	current_process->regs.useresp = 0xBFFFFFFB;
	current_process->regs.ebp = 0xBFFFFFFB;

	memset(0xBFFFF000, 0, 4092);

	tss_set_stack(current_process->kernel_stack);

	__asm__ __volatile__ (
			"push $0x23\n"			// User DS
			"mov %0, %%eax\n"
			"push %%eax\n"			// User stack
			"push $512\n"			// EFLAGS
			"push $0x1B\n"			// User CS
			"mov %1, %%eax\n"
			"push %%eax\n"			// User EIP
			"iret\n"
			:
			: "r"(current_process->regs.useresp), "r"(current_process->regs.eip)
			: "eax");

}

void syscall_fork(registers_state *regs) {
	process_t *process = malloc(sizeof(process_t));
	memset(process, 0, sizeof(process_t));

	// Deep copy of current process' directory
	void *new_page_dir_phys = allocate_blocks(1);
	map_page(new_page_dir_phys, 0xE0000000); // Temporary mapping
	memset(0xE0000000, 0, 4096);
	page_directory_t *pd = (page_directory_t *)0xE0000000;
	page_directory_t *cur_pd = (page_directory_t *)0xFFFFF000;

	// Link kernel pages 
	for (u32 i = 768; i < 1024; ++i) {
		pd->entries[i] = cur_pd->entries[i];
	}
	// Setup recursive paging
	pd->entries[1023] = (u32)new_page_dir_phys | PAGING_FLAG_PRESENT | PAGING_FLAG_WRITEABLE;

	// Deep copy user pages
	for (u32 i = 0; i < 768; ++i) {
		if (!cur_pd->entries[i]) {
			continue;
		}
		page_table_t *cur_table = (page_table_t *)(0xFFC00000 + (i << 12));
		page_table_t *new_table_phys = allocate_blocks(1);
		page_table_t *new_table = (page_table_t *)0xEA000000;
		map_page(new_table_phys, 0xEA000000); // Temporary mapping
		memset(0xEA000000, 0, 4096);
		for (u32 j = 0; j < 1024; ++j) {
			if (!cur_table->entries[j]) {
				continue;
			}
			// Copy the page frame's contents 
			page_table_entry cur_pte = cur_table->entries[j];
			u32 cur_page_frame = (u32)GET_FRAME(cur_pte);
			void *new_page_frame = allocate_blocks(1);
			map_page(cur_page_frame, 0xEB000000);
			map_page(new_page_frame, 0xEC000000);
			memcpy(0xEC000000, 0xEB000000, 4096);
			unmap_page(0xEC000000);
			unmap_page(0xEB000000);

			// Insert the corresponding page table entry
			page_table_entry new_pte = 0;
			if ((cur_pte & PAGING_FLAG_PRESENT) == PAGING_FLAG_PRESENT) {
				new_pte |= PAGING_FLAG_PRESENT;
			}
			if ((cur_pte & PAGING_FLAG_WRITEABLE) == PAGING_FLAG_WRITEABLE) {
				new_pte |= PAGING_FLAG_WRITEABLE;
			}
			if ((cur_pte & PAGING_FLAG_ACCESSED) == PAGING_FLAG_ACCESSED) {
				new_pte |= PAGING_FLAG_ACCESSED;
			}
			if ((cur_pte & PAGING_FLAG_DIRTY) == PAGING_FLAG_DIRTY) {
				new_pte |= PAGING_FLAG_DIRTY;
			}
			if ((cur_pte & PAGING_FLAG_USER) == PAGING_FLAG_USER) {
				new_pte |= PAGING_FLAG_USER;
			}
			new_pte = ((new_pte & ~0xFFFFF000) | (physical_address)new_page_frame);
			new_table->entries[j] = new_pte;
		}
		unmap_page(0xEA000000);

		// Insert the corresponding page directory entry
		page_directory_entry cur_pde = cur_pd->entries[i];
		page_directory_entry new_pde = 0;
		if ((cur_pde & PAGING_FLAG_PRESENT) == PAGING_FLAG_PRESENT) {
			new_pde |= PAGING_FLAG_PRESENT;
		}
		if ((cur_pde & PAGING_FLAG_WRITEABLE) == PAGING_FLAG_WRITEABLE) {
			new_pde |= PAGING_FLAG_WRITEABLE;
		}
		if ((cur_pde & PAGING_FLAG_ACCESSED) == PAGING_FLAG_ACCESSED) {
			new_pde |= PAGING_FLAG_ACCESSED;
		}
		if ((cur_pde & PAGING_FLAG_DIRTY) == PAGING_FLAG_DIRTY) {
			new_pde |= PAGING_FLAG_DIRTY;
		}
		if ((cur_pde & PAGING_FLAG_USER) == PAGING_FLAG_USER) {
			new_pde |= PAGING_FLAG_USER;
		}
		new_pde = ((new_pde & ~0xFFFFF000) | (physical_address)new_table_phys);
		pd->entries[i] = new_pde;
	}

	unmap_page(0xE0000000);

	process->next = process;
	process->directory = new_page_dir_phys;
	void *kernel_stack = malloc(4096);
	process->kernel_stack = (u8 *)kernel_stack + 4096 - 1;
	//memcpy(&process->regs, &current_process->regs, sizeof(registers_state));
	process->regs.eip = regs->eip;
	process->regs.cs = 0x1B;
	process->regs.ds = 0x23;
	process->regs.useresp = regs->useresp;
	process->regs.ebp = regs->ebp;
	process->pid = pid_gen++;

	process->regs.eax = 0; 
	current_process->regs.eax = process->pid; 

	if (current_process && current_process->next) {
		process_t *p = current_process->next;
		current_process->next = process;
		process->next = p;
	} else if (!current_process) {
		current_process = process;
		current_process->next = current_process;
	}

	regs->eax = process->pid;
}
