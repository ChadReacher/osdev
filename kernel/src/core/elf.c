#include <elf.h>
#include <string.h>
#include <paging.h>
#include <process.h>
#include <panic.h>
#include <blk_dev.h>
#include <heap.h>
#include <errno.h>

extern struct proc *current_process;
extern void irq_ret();

static void dump_elf_header(struct elf_header *elf_h);
static void dump_program_header(struct elf_program_header ph);

static i32 is_elf(struct elf_header *elf) {
	i32 ret = -1;
	if (elf->magic_number == ELF_MAGIC_NUMBER && strncmp((i8 *)elf->elf_ascii, "ELF", 3) == 0) {
		ret = 0;
	}
	if (ret != -1) {
		ret = elf->type;
	}
	return ret;
}

static void map_user_stack() {
	void *stack_frame;
	page_directory_entry *stack_pd_entry;
	page_table_t *stack_page_table;
	page_table_entry stack_page;
	page_directory_t *cur_pd = (page_directory_t *)0xFFFFF000;

	stack_frame = allocate_blocks(1);
	map_page(stack_frame, (void *)0xBFFFF000, PAGING_FLAG_PRESENT | PAGING_FLAG_WRITEABLE | PAGING_FLAG_USER);
	stack_pd_entry = &cur_pd->entries[767];
	*stack_pd_entry |= PAGING_FLAG_PRESENT;
	*stack_pd_entry |= PAGING_FLAG_WRITEABLE;
	*stack_pd_entry |= PAGING_FLAG_USER;
	stack_page_table = (page_table_t *)(0xFFC00000 + (PAGE_DIR_INDEX((u32)0xBFFFF000) << 12));
	stack_page = 0;
	stack_page |= PAGING_FLAG_PRESENT;
	stack_page |= PAGING_FLAG_WRITEABLE;
	stack_page |= PAGING_FLAG_USER;
	stack_page = ((stack_page & ~0xFFFFF000) | (physical_address)stack_frame);
	stack_page_table->entries[PAGE_TABLE_INDEX(0xBFFFF000)] = stack_page;
}

i8 *setup_user_stack(i32 argc, i8 **argv, i32 envc, i8 **envp) {
	i32 i, env_ptr, arg_ptr;
	i8 *usp;

	map_user_stack();

	memset((void *)0xBFFFF000, 0, 4092);
	usp = (i8 *)0xBFFFFFFB;
	/* push envp strings */
	for (i = envc - 1; i >= 0; --i) {
		usp -= strlen(envp[i]) + 1;
		strcpy((i8 *)usp, envp[i]);
		free(envp[i]);
		envp[i] = (i8 *)usp;
	}
	/* push argv strings */
	for (i = argc - 1; i >= 0; --i) {
		usp -= strlen(argv[i]) + 1;
		strcpy((i8 *)usp, argv[i]);
		free(argv[i]);
		argv[i] = (i8 *)usp;
	}
	/* Push envp pointers to envp strings */
	usp -= (envc + 1) * 4;
	memcpy((void *)usp, (void *)envp, (envc + 1) * 4);
	/* Save env ptr */
	env_ptr = (u32)usp;
	/* Push argv pointers argv strings */
	usp -= (argc + 1) * 4;
	memcpy((void *)usp, (void *)argv, (argc + 1) * 4);
	/* Save arg ptr */
	arg_ptr = (u32)usp;
	usp -= 4;
	*((u32*)usp) = env_ptr;
	usp -= 4;
	*((u32*)usp) = arg_ptr;
	usp -= 4;
	*((u32*)usp) = argc;

	free(argv);
	free(envp);
	return usp;
}

static void setup_heap(u32 last_addr) {
	void *brk_phys_frame;

	brk_phys_frame = allocate_blocks(1);
	current_process->brk = ALIGN_UP(last_addr, 4096);
	map_page(brk_phys_frame, (void *)current_process->brk, PAGING_FLAG_PRESENT | PAGING_FLAG_WRITEABLE | PAGING_FLAG_USER);
}

static void setup_kernel_stack(i8 *usp) {
	u32 *sp;

	sp = (u32 *)ALIGN_DOWN((u32)current_process->kernel_stack_bottom + 4096 * 2 - 1, 4);
	/* Setup kernel stack as we have returned from interrupt routine */
	*sp-- = 0x23;			/* user DS */
	*sp-- = (u32)usp;		/* user stack */
	*sp-- = 0x200;			/* EFLAGS */
	*sp-- = 0x1B;			/* user CS */
	*sp-- = 0x0;			/* user eip */
	*sp-- = 0x0;			/* err code */
	*sp-- = 0x0;			/* int num */
	*sp-- = 0x0;			/* eax */
	*sp-- = 0x0; 			/* ecx */
	*sp-- = 0x0; 			/* edx */
	*sp-- = 0x0; 			/* ebx */
	*sp-- = 0x0; 			/* esp */
	*sp-- = 0x0;			/* ebp */
	*sp-- = 0x0; 			/* esi */
	*sp-- = 0x0; 			/* edi */
	*sp-- = 0x23;			/* ds */
	*sp-- = 0x23; 			/* es */
	*sp-- = 0x23; 			/* fs */
	*sp-- = 0x23; 			/* gs */
	*current_process->regs = *((struct registers_state *)(sp + 1));
	*sp-- = (u32)irq_ret;	/* irq_ret eip (to return back to the end of the interrupt routine) */
	*sp-- = 0x0;			/* ebp */
	*sp-- = 0x0; 			/* ebx */
	*sp-- = 0x0; 			/* esi */
	*sp-- = 0x0; 			/* edi */
	++sp;
	current_process->kernel_stack_top = (void *)sp;
	current_process->context = (struct context *)sp;
}

i32 elf_load(struct ext2_inode *inode, 
		i32 argc, i8 **argv, i32 envc, i8 **envp) {
	i8 *usp;
	i32 i;
	u32 last_addr;
	struct elf_header elf_header;
	page_directory_entry *code_pd_entry;
	page_directory_t *cur_pd = (page_directory_t *)0xFFFFF000;
	struct file fp;

	fp.f_mode = inode->i_mode;
	fp.f_flags = 0;
	fp.f_count = 1;
	fp.f_inode = inode;
	fp.f_pos = 0;
	if (ext2_file_read(inode, &fp, (i8 *)&elf_header, sizeof(struct elf_header))
				!= sizeof(struct elf_header)) {
		return -ENOEXEC;
	}
	if (is_elf(&elf_header) != ET_EXEC) {
		return -ENOEXEC;
	}
	dump_elf_header(&elf_header);
	free_user_image();
	code_pd_entry = &cur_pd->entries[0];
	*code_pd_entry |= PAGING_FLAG_USER;
	*code_pd_entry |= PAGING_FLAG_WRITEABLE;
	for (i = 0; i < elf_header.ph_num; ++i) {
		u32 j, addr;
		u32 blocks, flags;
		void *code_phys_frame;
		struct elf_program_header program_header;

		fp.f_pos = elf_header.phoff + elf_header.ph_size * i;
		if (ext2_file_read(inode, &fp, (i8 *)&program_header, 
			sizeof(struct elf_program_header)) != sizeof(struct elf_program_header)) {
			return -ENOEXEC;
		}
		dump_program_header(program_header);

		flags = PAGING_FLAG_PRESENT | PAGING_FLAG_USER;
		flags |= (program_header.flags & PF_W) ? PAGING_FLAG_WRITEABLE : 0;
		flags |= (program_header.flags & PF_X) ? 0 : 0;
		flags |= (program_header.flags & PF_R) ? 0 : 0;
		if (program_header.memsz == 0) {
			continue;
		}
		blocks = program_header.memsz / 4096;
		if (program_header.memsz % 4096 != 0) {
			++blocks;
		}
		code_phys_frame = allocate_blocks(blocks);
		/* Map necessary pages for code */
		for (j = 0, addr = program_header.vaddr; j < blocks; ++j, addr += 0x1000) {
			u32 phys_code_page = (u32)code_phys_frame + j * 0x1000;
			map_page((void *)phys_code_page, (void *)addr, flags);
		}
		fp.f_pos = program_header.offset;
		ext2_file_read(inode, &fp, (i8 *)program_header.vaddr,
				program_header.filesz);
		memset((void *)(program_header.vaddr + program_header.filesz),
				0, program_header.memsz - program_header.filesz);
		last_addr = program_header.vaddr + program_header.memsz;
	}
	usp = setup_user_stack(argc, argv, envc, envp);
	setup_heap(last_addr);
	setup_kernel_stack(usp);
	return 0;
}

static void dump_elf_header(struct elf_header *elf_h) {
	debug("magic_number - %x\r\n", elf_h->magic_number);
	debug("magic_number - %x\r\n", elf_h->magic_number);
	debug("elf_ascii[3] - %c, %c, %c\r\n", elf_h->elf_ascii[0], elf_h->elf_ascii[1], elf_h->elf_ascii[2]);
	debug("class - %x\r\n", elf_h->class);
	debug("data - %x\r\n", elf_h->data);
	debug("version - %x\r\n", elf_h->version);
	debug("os_abi - %x\r\n", elf_h->os_abi);
	debug("padding[8] - [0]....8 times");
	debug("type - %x\r\n", elf_h->type);
	debug("machine - %x\r\n", elf_h->machine);
	debug("elf_version - %x\r\n", elf_h->elf_version);
	debug("entry - %x\r\n", elf_h->entry);
	debug("phoff - %x\r\n", elf_h->phoff);
	debug("shoff - %x\r\n", elf_h->shoff);
	debug("flags - %x\r\n", elf_h->flags);
	debug("header_size - %x\r\n", elf_h->header_size);
	debug("ph_size - %x\r\n", elf_h->ph_size);
	debug("ph_num - %x\r\n", elf_h->ph_num);
	debug("sh_size - %x\r\n", elf_h->sh_size);
	debug("sh_num - %x\r\n", elf_h->sh_num);
	debug("strtab_idx - %x\r\n", elf_h->strtab_idx);
}

static void dump_program_header(struct elf_program_header ph) {
	debug("type - %x\r\n", ph.type);
	debug("offset - %x\r\n", ph.offset);
	debug("vaddr - %x\r\n", ph.vaddr);
	debug("paddr - %x\r\n", ph.paddr);
	debug("filesz - %x\r\n", ph.filesz);
	debug("memsz - %x\r\n", ph.memsz);
	debug("flags - %x\r\n", ph.flags);
	debug("align - %x\r\n", ph.align);
}
