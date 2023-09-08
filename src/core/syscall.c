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

extern process_t *init_process;
extern process_t *proc_list;
extern process_t *current_process;
extern void irq_ret();

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
	syscall_register_handler(SYSCALL_WAITPID, syscall_waitpid);
	syscall_register_handler(SYSCALL_GETPID, syscall_getpid);
	syscall_register_handler(SYSCALL_DUP, syscall_dup);
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
	DEBUG("%s", "exec\r\n");
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
					memset((void *)addr, 0, 0x1000);
					memcpy((void *)addr, code + addr, 0x1000);
				}
			}
		}
	}
	free(data);
	// Flush TLB
	__asm__ __volatile__ ("movl %%cr3, %%eax" : : );
	__asm__ __volatile__ ("movl %%eax, %%cr3" : : );


	void *old_kernel_stack = current_process->kernel_stack_bottom;
	void *kernel_stack = malloc(4096);
	memset(kernel_stack, 0, 4096);
	current_process->kernel_stack_bottom = kernel_stack;
	free(old_kernel_stack);

	memset(0xBFFFF000, 0, 4092);

	u32 *sp = (u32 *)ALIGN_DOWN((u32)current_process->kernel_stack_bottom + 4096 - 1, 4);
	// Setup kernel stack as we have returned from interrupt routine
	*sp-- = 0x23;			// user DS
	*sp-- = 0xBFFFFFFB;		// user stack
	*sp-- = 0x200;			// EFLAGS
	*sp-- = 0x1B;			// user CS
	*sp-- = 0x0;			// user eip
	*sp-- = 0x0;			// err code
	*sp-- = 0x0;			// int num
	*sp-- = 0x0;			// eax
	*sp-- = 0x0; 			// ecx
	*sp-- = 0x0; 			// edx
	*sp-- = 0x0; 			// ebx
	*sp-- = 0x0; 			// esp
	*sp-- = 0x0;			// ebp
	*sp-- = 0x0; 			// esi
	*sp-- = 0x0; 			// edi
	*sp-- = 0x23;			// ds
	*sp-- = 0x23; 			// es
	*sp-- = 0x23; 			// fs
	*sp-- = 0x23; 			// gs
	current_process->regs = (registers_state *)(sp + 1);
	*sp-- = (u32)irq_ret;	// irq_ret eip (to return back to the end of the interrupt routine)
	*sp-- = 0x0;			// ebp
	*sp-- = 0x0; 			// ebx
	*sp-- = 0x0; 			// esi
	*sp-- = 0x0; 			// edi
	++sp;

	current_process->kernel_stack_top = (void *)sp;
	current_process->context = (context_t *)sp;
	free(current_process->cwd);
	current_process->cwd = strdup("/");
	current_process->timeslice = 20;
	current_process->priority = 20;
	DEBUG("%s", "End of exec\r\n");
	// It fixes problem, but it's strange
	*regs = *current_process->regs;
}

void syscall_fork(registers_state *regs) {
	DEBUG("%s", "forked()\r\n");
	process_t *process = proc_alloc();

	process->directory = paging_copy_page_dir(true);
	process->cwd = strdup(current_process->cwd);
	process->parent = current_process;
	process->fds = malloc(FDS_NUM * sizeof(file));
	memcpy(process->fds, current_process->fds, FDS_NUM * sizeof(file));
	process->timeslice = 20;
	process->priority = 20;

	*process->regs = *current_process->regs;

	process->regs->eax = 0; 
	current_process->regs->eax = process->pid; 
}

void syscall_exit(registers_state *regs) {
	i32 exit_code = (i32)regs->ebx;

	if (current_process->pid == 1) {
		PANIC("Can't exit the INIT process\r\n");
	}
	DEBUG("exit code - %d\r\n", exit_code);

	// Free the user code pages in the page directory
	void *page_dir_phys = (void *)current_process->directory;
	map_page(page_dir_phys, 0xE0000000);
	page_directory_t *page_dir = (page_directory_t *)0xE0000000;
	for (u32 i = 0; i < 768; ++i) {
		if (!page_dir->entries[i]) {
			continue;
		}
		page_directory_entry pde = page_dir->entries[i];
		void *table_phys = (void *)GET_FRAME(pde);
		map_page(table_phys, 0xEA000000);
		page_table_t *table = (page_table_t *)0xEA000000;
		for (u32 j = 0; j < 1024; ++j) {
			if (!table->entries[j]) {
				continue;
			}
			page_table_entry pte = table->entries[j];
			void *page_frame = (void *)GET_FRAME(pte);
			free_blocks(page_frame, 1);
		}
		unmap_page(0xEA000000);
		free_blocks(table_phys, 1);
	}
	unmap_page(0xE0000000);
	free_blocks(page_dir_phys, 1);
	
	// Flush TLB
	__asm__ __volatile__ ("movl %%cr3, %%eax" : : );
	__asm__ __volatile__ ("movl %%eax, %%cr3" : : );

	// Pass current_process's children to INIT process
	for (process_t *p = proc_list; p != NULL; p = p->next) {
		if (p->state == RUNNING && p->parent == current_process) {
			p->parent = init_process;
			if (p->state == ZOMBIE) {
				wakeup(init_process);
			}
		}
	}

	// Close open files
	for (u32 i = 3; i < FDS_NUM; ++i) {
		file *f = &current_process->fds[i];
		if (f && f->vfs_node && f->vfs_node != (void *)-1) {
			vfs_close(f->vfs_node);
			memset(f, 0, sizeof(file));
		}
	}
	free(current_process->fds);
	free(current_process->cwd);

	current_process->exit_code = exit_code;

	wakeup(current_process->parent);
	
	// TODO: Should we maintain another list for zombie processes or
	// just keep the running list and list of ALL processes?
	// remove_process_from_list(current_process)

	current_process->state = ZOMBIE;
	schedule(NULL);

	PANIC("Zombie returned from scheduler\r\n");
}

void syscall_waitpid(registers_state *regs) {
	i32 pid = (i32)regs->ebx;
	i32 *wstatus = (i32 *)regs->ecx;
	i32 options = (i32)regs->edx;

	if (pid < -1) {
		// Wait for any child process whose process group ID
		// is equalt to the absolute value of 'pid'
	} else if (pid == -1) {
		// Wait for any child process
		for (;;) {
			bool havekids = false;
			for (process_t *p = proc_list; p != NULL; p = p->next) {
				if (p->parent != current_process) {
					continue;
				}
				havekids = true;
				if (p->state ==	ZOMBIE) {
					if (wstatus) {
						*wstatus = (p->exit_code << 8) & 0x7F;
					}
					u32 ret_pid = p->pid;
					free(p->kernel_stack_bottom);
					remove_process_from_list(p);
					current_process->regs->eax = ret_pid;
					return;
				}
			}
			if (!havekids) {
				return -1;
			}

			sleep(current_process);
		}
	} else if (pid == 0) {
		// Wait for any child process whose process group ID
		// is equal to that of the calling process at the time
		// of the call to 'waitpid()'
	} else if (pid > 0) {
		// Wait for the child whose process ID is equal to the value of 'pid'
	}
}

void syscall_getpid(registers_state *regs) {
	regs->eax = current_process->pid;
}

void syscall_dup(registers_state *regs) {
	i32 oldfd = (i32)regs->ebx;

	if (oldfd > FDS_NUM || current_process->fds[oldfd].used || current_process->fds[oldfd].vfs_node == (void *) -1) {
		regs->eax = -1;
		return;
	}

	i32 newfd = proc_get_fd(current_process);
	if (newfd < 0) {
		regs->eax = -1;
		return;
	}
	current_process->fds[newfd] = current_process->fds[oldfd];
	regs->eax = newfd;
}
