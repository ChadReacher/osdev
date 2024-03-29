#include <syscall.h>
#include <stdio.h>
#include <panic.h>
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
#include <pmm.h>
#include <idt.h>
#include <heap.h>


extern process_t *init_process;
extern process_t *proc_list;
extern process_t *current_process;
extern void irq_ret();

syscall_handler_t syscall_handlers[NB_SYSCALLS] = {
	syscall_test,
	syscall_read,
	syscall_write,
	syscall_open,
	syscall_close,
	syscall_lseek,
	syscall_unlink,
	syscall_yield,
	syscall_exec,
	syscall_fork,
	syscall_exit,
	syscall_waitpid,
	syscall_getpid,
	syscall_dup,
	syscall_sbrk,
	syscall_nanosleep,
	syscall_getcwd,
	syscall_fstat,
	syscall_chdir
};

void syscall_init() {
	idt_set(SYSCALL, (u32)isr0x80, 0xEE);
}

void syscall_register_handler(u8 id, syscall_handler_t handler) {
	syscall_handlers[id] = handler;
}

i32 syscall_handler(registers_state *regs) {
	syscall_handler_t handler = syscall_handlers[regs->eax];

	if (handler != 0) {
		return handler(regs);
	}

	PANIC("Received unimplemented syscall: %d\n", regs->eax);
	return -1;
}


i32 syscall_test(registers_state *regs) {
	kprintf("Hello from syscall_test(), %s\n", regs->ebx);
	return 0;
}

i32 syscall_open(registers_state *regs) {
	i8 *filename = (i8* )regs->ebx;
	u32 oflags = regs->ecx;
	u32 mode = regs->edx;

	vfs_node_t *vfs_node;

	vfs_node = vfs_get_node(filename);
	if (!vfs_node) {
		if (oflags & O_CREAT) {
			vfs_create(filename, mode);
			vfs_node = vfs_get_node(filename);
		} else {
			return -1;
		}
	} else if ((vfs_node->flags & FS_DIRECTORY) == FS_DIRECTORY) {
		//DEBUG("Cannot open a directory - %s\r\n", filename);
		//return -1;
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
	return fd;
}

i32 syscall_close(registers_state *regs) {
	i32 fd = (i32)regs->ebx;
	if (fd < 3 || fd >= FDS_NUM) {
		DEBUG("Invalid file descriptor - %d\r\n", fd);
		return -1;
	}
	file *f = &current_process->fds[fd];
	if (!f->used || !f->vfs_node) {
		DEBUG("Bad descriptor - %d\r\n", fd);
		return -1;
	}
	vfs_close(f->vfs_node);
	free(f->vfs_node);
	memset(f, 0, sizeof(file));
	return 0;
}

i32 syscall_read(registers_state *regs) {
	i32 fd = (i32)regs->ebx;
	i8 *buf = (i8 *)regs->ecx;
	u32 count = (u32)regs->edx;

	if (fd == FD_STDIN) {
		u8 c = keyboard_getchar();
		if (c) {
			*((u8 *)buf) = c;
			return 1;
		}
		return 0;
	}

	if (fd < 3 || fd >= FDS_NUM) {
		DEBUG("Invalid file descriptor - %d\r\n", fd);
		return 0;
	}

	file *f = &current_process->fds[fd];
	if (!f->used || !f->vfs_node) {
		DEBUG("Bad descriptor - %d\r\n", fd);
		return 0;
	}
	if (f->flags & O_WRONLY) {
		DEBUG("%s", "Cannot read file with O_WRONLY flag\r\n");
		return 0;
	}
	u32 have_read;
	if ((f->vfs_node->flags & FS_DIRECTORY) == FS_DIRECTORY) {
		u32 idx = f->offset / sizeof(dirent);
		dirent *dent = vfs_readdir(f->vfs_node, idx);
		if (dent == NULL) {
			return 0;
		}
		memcpy((void *)buf, (void *)dent, sizeof(dirent));
		free(dent);
		f->offset += sizeof(dirent);
		have_read = sizeof(dirent);
	} else {
		have_read = vfs_read(f->vfs_node, f->offset, count, buf);
		f->offset += have_read;
	}

	return have_read;
}

i32 syscall_write(registers_state *regs) {
	i32 fd = regs->ebx;
	i8 *buf = (i8 *)regs->ecx;
	u32 count = regs->edx;

	if (fd == FD_STDOUT || fd == FD_STDERR) {
		u32 i;
		for (i = 0; i < count; ++i) {
			screen_print_char(buf[i]);
		}
		return i;
	}

	if (fd < 3 || fd >= FDS_NUM) {
		DEBUG("Invalid file descriptor - %d\r\n", fd);
		return 0;
	}

	file *f = &current_process->fds[fd];

	if (!f->used || !f->vfs_node) {
		DEBUG("Bad descriptor - %d\r\n", fd);
		return 0;
	}
	if (!(f->flags & (O_WRONLY | O_RDWR))) {
		DEBUG("%s", "File is not opened for writing.\r\n");
		return 0;
	}
	if ((f->flags & O_APPEND) == O_APPEND) {
		f->offset = f->vfs_node->length;
	}
	u32 have_written = vfs_write(f->vfs_node, f->offset, count, buf);
	f->offset += have_written;
	return have_written;
}

i32 syscall_lseek(registers_state *regs) {
	i32 fd = regs->ebx;
	i32 offset = regs->ecx;
	i32 whence = regs->edx;

	if (fd < 3 || fd >= FDS_NUM) {
		DEBUG("Invalid file descriptor - %d\r\n", fd);
		return 0;
	}

	file *f = &current_process->fds[fd];

	if (!f->used || !f->vfs_node) {
		DEBUG("Bad descriptor - %d\r\n", fd);
		return 0;
	}
	if (whence == SEEK_SET) {
		f->offset = offset;
	} else if (whence == SEEK_CUR) {
		f->offset += offset;
	} else if (whence == SEEK_END) {
		f->offset = f->vfs_node->length + offset;
	} else {
		DEBUG("Invalid whence argument - %d\r\n", whence);
		return -1;
	}

	return f->offset;
}

i32 syscall_unlink(registers_state *regs) {
	i8 *filename = (i8 *)regs->ebx;
	i32 ret = vfs_unlink(filename);
	return ret;
}

i32 syscall_yield(registers_state *regs) {
	schedule(regs);
	return 0;
}

i32 syscall_exec(registers_state *regs) {
	i8 *pathname = (i8 *)regs->ebx;
	i8 **u_argv = (i8 **)regs->ecx;
	i8 **u_envp = (i8 **)regs->edx;

	i32 argc = 0, envc = 0;
	if (u_argv) {
		for (i8 **arg_p = u_argv; *arg_p; ++arg_p) {
			++argc;
		}
	}
	if (u_envp) {
		for (i8 **env_p = u_envp; *env_p; ++env_p) {
			++envc;
		}
	}

	i8 **argv = (i8 **)malloc((argc + 1) * sizeof(i8 *));
	i8 **envp = (i8 **)malloc((envc + 1) * sizeof(i8 *));

	argv[argc] = NULL;
	envp[envc] = NULL;

	for (i32 i = 0; i < argc; ++i) {
		argv[i] = strdup(u_argv[i]);
	}
	for (i32 i = 0; i < envc; ++i) {
		envp[i] = strdup(u_envp[i]);
	}
		
	vfs_node_t *vfs_node = vfs_get_node(pathname);
	if (!vfs_node || (vfs_node->flags & FS_FILE) != FS_FILE) {
		for (i32 i = 0; i < argc; ++i) {
			free(argv[i]);
		}
		for (i32 i = 0; i < envc; ++i) {
			free(envp[i]);
		}
		free(argv);
		free(envp);
		return -1;
	}


	u32 *data = (u32 *)malloc(vfs_node->length);
	memset((i8 *)data, 0, vfs_node->length);
	vfs_read(vfs_node, 0, vfs_node->length, (i8 *)data);

	void *prev_page_dir = current_process->directory;
	void *new_page_dir_phys = paging_copy_page_dir(false);
	current_process->directory = new_page_dir_phys;
	__asm__ __volatile__ ("movl %%eax, %%cr3" : : "a"((u32)new_page_dir_phys));

	elf_load(data);
	free(data);

	void *old_kernel_stack = current_process->kernel_stack_bottom;
	void *kernel_stack = malloc(4096 * 2);
	memset(kernel_stack, 0, 4096 * 2);
	current_process->kernel_stack_bottom = kernel_stack;
	free(old_kernel_stack);

	// Handle user stack:
	memset((void *)0xBFFFF000, 0, 4092);
	i8 *usp = (i8 *)0xBFFFFFFB;
	// push envp strings
	for (i32 i = envc - 1; i >= 0; --i) {
		usp -= strlen(envp[i]) + 1;
		strcpy((i8 *)usp, envp[i]);
		free(envp[i]);
		envp[i] = (i8 *)usp;
	}
	// push argv strings
	for (i32 i = argc - 1; i >= 0; --i) {
		usp -= strlen(argv[i]) + 1;
		strcpy((i8 *)usp, argv[i]);
		free(argv[i]);
		argv[i] = (i8 *)usp;
	}

	// Push envp pointers to envp strings
	usp -= (envc + 1) * 4;
	memcpy((void *)usp, (void *)envp, (envc + 1) * 4);

	// Save env ptr
	u32 env_ptr = (u32)usp;

	// Push argv pointers argv strings
	usp -= (argc + 1) * 4;
	memcpy((void *)usp, (void *)argv, (argc + 1) * 4);

	// Save arg ptr
	u32 arg_ptr = (u32)usp;

	usp -= 4;
	*((u32*)usp) = env_ptr;

	usp -= 4;
	*((u32*)usp) = arg_ptr;

	usp -= 4;
	*((u32*)usp) = argc;

	free(argv);
	free(envp);

	u32 *sp = (u32 *)ALIGN_DOWN((u32)current_process->kernel_stack_bottom + 4096 * 2 - 1, 4);
	// Setup kernel stack as we have returned from interrupt routine
	*sp-- = 0x23;			// user DS
	*sp-- = (u32)usp;		// user stack
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
	current_process->timeslice = 20;
	current_process->priority = 20;
	// It fixes problem, but it's strange
	*regs = *current_process->regs;

	// Free the user code pages in the page directory
	void *page_dir_phys = (void *)prev_page_dir;
	map_page(page_dir_phys, (void *)0xE0000000, PAGING_FLAG_PRESENT | PAGING_FLAG_WRITEABLE);
	page_directory_t *page_dir = (page_directory_t *)0xE0000000;
	for (u32 i = 0; i < 768; ++i) {
		if (!page_dir->entries[i]) {
			continue;
		}
		page_directory_entry pde = page_dir->entries[i];
		void *table_phys = (void *)GET_FRAME(pde);
		map_page(table_phys, (void *)0xEA000000, PAGING_FLAG_PRESENT | PAGING_FLAG_WRITEABLE);
		page_table_t *table = (page_table_t *)0xEA000000;
		for (u32 j = 0; j < 1024; ++j) {
			if (!table->entries[j]) {
				continue;
			}
			page_table_entry pte = table->entries[j];
			void *page_frame = (void *)GET_FRAME(pte);
			free_blocks(page_frame, 1);
		}
		memset(table, 0, 4096);
		unmap_page((void *)0xEA000000);
		free_blocks(table_phys, 1);
	}
	memset(page_dir, 0, 4096);
	unmap_page((void *)0xE0000000);
	free_blocks(page_dir_phys, 1);
	
	// Flush TLB
	__asm__ __volatile__ ("movl %%cr3, %%eax" : : );
	__asm__ __volatile__ ("movl %%eax, %%cr3" : : );

	return 0;
}

i32 syscall_fork(registers_state *regs) {
	(void)regs;

	process_t *process = proc_alloc();

	process->directory = paging_copy_page_dir(true);
	process->cwd = strdup(current_process->cwd);
	process->parent = current_process;
	process->timeslice = 20;
	process->priority = 20;
	process->brk = current_process->brk;

	*process->regs = *current_process->regs;

	process->regs->eax = 0; 
	return process->pid;
}

i32 syscall_exit(registers_state *regs) {
	i32 exit_code = (i32)regs->ebx;

	if (current_process->pid == 1) {
		PANIC("Can't exit the INIT process\r\n");
	}

	// Free the user code pages in the page directory
	void *page_dir_phys = (void *)current_process->directory;
	map_page(page_dir_phys, (void *)0xE0000000, PAGING_FLAG_PRESENT | PAGING_FLAG_WRITEABLE);
	page_directory_t *page_dir = (page_directory_t *)0xE0000000;
	for (u32 i = 0; i < 768; ++i) {
		if (!page_dir->entries[i]) {
			continue;
		}
		page_directory_entry pde = page_dir->entries[i];
		void *table_phys = (void *)GET_FRAME(pde);
		map_page(table_phys, (void *)0xEA000000, PAGING_FLAG_PRESENT | PAGING_FLAG_WRITEABLE);
		page_table_t *table = (page_table_t *)0xEA000000;
		for (u32 j = 0; j < 1024; ++j) {
			if (!table->entries[j]) {
				continue;
			}
			page_table_entry pte = table->entries[j];
			void *page_frame = (void *)GET_FRAME(pte);
			free_blocks(page_frame, 1);
		}
		memset(table, 0, 4096);
		unmap_page((void *)0xEA000000);
		free_blocks(table_phys, 1);
	}
	memset(page_dir, 0, 768 * 4);
	unmap_page((void *)0xE0000000);
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
			free(f->vfs_node);
			memset(f, 0, sizeof(file));
		}
	}
	free(current_process->fds);
	free(current_process->cwd);

	current_process->exit_code = exit_code;

	wakeup(current_process->parent);
	
	current_process->state = ZOMBIE;
	schedule(NULL);

	PANIC("Zombie returned from scheduler\r\n");
	return 0;
}

i32 syscall_waitpid(registers_state *regs) {
	i32 pid = (i32)regs->ebx;
	i32 *wstatus = (i32 *)regs->ecx;
	i32 options = (i32)regs->edx;
	(void)options;

	if (pid < -1) {
		// Wait for any child process whose process group ID
		// is equal to the absolute value of 'pid'
		return 0;
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
						*wstatus = p->exit_code & 0xFF;
					}
					u32 ret_pid = p->pid;
					free(p->kernel_stack_bottom);
					remove_process_from_list(p);
					free(p);
					return ret_pid;
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
		return 0;
	} else if (pid > 0) {
		// Wait for the child whose process ID is equal to the value of 'pid'
		return 0;
	}
	return -1;
}

i32 syscall_getpid(registers_state *regs) {
	(void)regs;
	return current_process->pid;
}

i32 syscall_dup(registers_state *regs) {
	i32 oldfd = (i32)regs->ebx;

	if (oldfd > FDS_NUM || current_process->fds[oldfd].used) {
		return -1;
	}

	i32 newfd = proc_get_fd(current_process);
	if (newfd < 0) {
		return -1;
	}
	current_process->fds[newfd] = current_process->fds[oldfd];
	return newfd;
}

i32 syscall_sbrk(registers_state *regs) {
	u32 incr = (u32)regs->ebx; 
	u32 old_brk = current_process->brk;

	if (!incr) {
		return old_brk;
	}

	current_process->brk += incr;
	return old_brk;
}

i32 syscall_nanosleep(registers_state *regs) {
	const struct timespec *req = (const struct timespec *)regs->ebx;
	const struct timespec *rem = (const struct timespec *)regs->ecx;
	(void)rem;

	u32 nsec = req->tv_nsec;
	if (nsec < 10000000) {
		nsec *= 10;
	}

	u32 timeout = (req->tv_sec * 100) + (nsec * 100 / 1000000000L);
	if (timeout) {
		current_process->timeout = timeout;
		sleep((void *)&syscall_nanosleep);
	}
	return 0;
}

i32 syscall_getcwd(registers_state *regs) {
	i8 *buf = (i8 *)regs->ebx;
	u32 size = (u32)regs->ecx;

	if (!size) {
		return -1;
	}

	u32 len = strlen(current_process->cwd);
	if (len + 1 > size) {
		return -1;
	}

	memcpy(buf, current_process->cwd, len + 1);
	return 0;
}

i32 syscall_fstat(registers_state *regs) {
	i32 fd = (i32)regs->ebx;
	struct stat *statbuf = (struct stat *)regs->ecx;

	if (current_process->fds[fd].vfs_node == (void *)-1) {
		return -1;
	}

	vfs_node_t *vfs_node = current_process->fds[fd].vfs_node;
	statbuf->st_dev = 0;
	statbuf->st_ino = vfs_node->inode;
	statbuf->st_mode = vfs_node->permission_mask;
	if ((vfs_node->flags & FS_FILE) == FS_FILE) {
		statbuf->st_mode |= S_IFREG;
	}
	if ((vfs_node->flags & FS_DIRECTORY) == FS_DIRECTORY) {
		statbuf->st_mode |= S_IFDIR;
	}
	if ((vfs_node->flags & FS_CHARDEVICE) == FS_CHARDEVICE) {
		statbuf->st_mode |= S_IFCHR;
	}
	if ((vfs_node->flags & FS_BLOCKDEVICE) == FS_BLOCKDEVICE) {
		statbuf->st_mode |= S_IFBLK;
	}
	if ((vfs_node->flags & FS_PIPE) == FS_PIPE) {
		statbuf->st_mode |= S_IFIFO;
	}
	if ((vfs_node->flags & FS_SYMLINK) == FS_SYMLINK) {
		statbuf->st_mode |= S_IFLNK;
	}
	statbuf->st_nlink = 0;
	statbuf->st_uid = vfs_node->uid;
	statbuf->st_gid = vfs_node->gid;
	statbuf->st_rdev = 0;
	statbuf->st_size = vfs_node->length;
	statbuf->st_atime = vfs_node->atime;
	statbuf->st_mtime = vfs_node->mtime;
	statbuf->st_ctime = vfs_node->ctime;
	statbuf->st_blksize = 0;
	statbuf->st_blocks = 0;
	return 0;
}

// Caller should free the memory
static i8 *make_absolute_path(i8 *rel_path) {
       i8 *cwd = current_process->cwd;
       i8 *abs_path = malloc(strlen(cwd) + 1 + strlen(rel_path) + 1);
       memset(abs_path, 0, strlen(cwd) + 1 + strlen(rel_path) + 1);

       memcpy(abs_path, cwd, strlen(cwd));

       if (strlen(cwd) != 1) {
               abs_path[strlen(abs_path)] = '/';
       }
       strcat(abs_path, rel_path);

       i8 *canonilized_path = canonilize_path(abs_path);
       if (abs_path) {
               free(abs_path);
       }

       return canonilized_path;
}

i32 syscall_chdir(registers_state *regs) {
	i8 *path = (i8 *)regs->ebx;
	
	i8 *abs_path;
	if (path[0] == '/') {
		abs_path = strdup(path);
	} else {
		abs_path = make_absolute_path(path);
	}

	vfs_node_t *vfs_node = vfs_get_node(abs_path);
	if (!vfs_node || (vfs_node->flags & FS_DIRECTORY) != FS_DIRECTORY) {
		free(abs_path);
		return -1;
	}

	free(current_process->cwd);
	current_process->cwd = abs_path;
	return 0;
}

