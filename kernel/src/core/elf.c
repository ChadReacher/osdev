#include <elf.h>
#include <string.h>
#include <paging.h>
#include <process.h>
#include <panic.h>
#include <blk_dev.h>
#include <heap.h>
#include <errno.h>
#include <pmm.h>
#include <binfmt.h>

#define USER_STACK_TOP KERNEL_LOAD_VADDR
#define USER_STACK_SIZE (2 * PAGE_SIZE)
#define USER_STACK_BASE (USER_STACK_TOP - USER_STACK_SIZE)

extern void irq_ret();

static void dump_elf_header(struct elf_header *elf_h);
static void dump_program_header(struct elf_program_header ph);

static inline i32 is_elf(struct elf_header *hdr) {
	return hdr->magic_number == ELF_MAGIC_NUMBER && strncmp((i8 *)hdr->elf_ascii, "ELF", 3) == 0;
}

static i32 map_user_stack() {
	u32 page = USER_STACK_BASE;
	u32 pages = USER_STACK_SIZE / PAGE_SIZE;
	const u32 flags = PAGING_FLAG_PRESENT | PAGING_FLAG_WRITEABLE | PAGING_FLAG_USER;
	for (u32 i = 0; i < pages; ++i) {
		void *frame = allocate_blocks(1);
		if (!frame) {
			return -1;
		}
		map_page((physical_address)frame, page, flags);
		page += PAGE_SIZE;
	}
	return 0;
}

i8 *setup_user_stack(i32 argc, i8 **argv, i32 envc, i8 **envp) {
	i32 env_ptr, arg_ptr;
	i8 *usp;

	if (map_user_stack()) {
		return NULL;
        }

	memset((void *)USER_STACK_BASE, 0, USER_STACK_SIZE);
	usp = (i8 *)USER_STACK_TOP - sizeof(i32);

	/* push envp strings */
	for (i32 i = envc - 1; i >= 0; --i) {
		usp -= strlen(envp[i]) + 1;
		strcpy((i8 *)usp, envp[i]);
		free(envp[i]);
		envp[i] = (i8 *)usp;
	}
	/* push argv strings */
	for (i32 i = argc - 1; i >= 0; --i) {
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

static i32 setup_heap(u32 last_addr) {
	const u32 flags = PAGING_FLAG_PRESENT | PAGING_FLAG_WRITEABLE | PAGING_FLAG_USER; 
	void *brk_phys_frame = allocate_blocks(1);
	if (brk_phys_frame == NULL) {
		return -1;
	}
	current_process->brk = ALIGN_UP(last_addr, PAGE_SIZE);
	map_page((physical_address)brk_phys_frame, current_process->brk, flags);
	return 0;
}

/* TODO: REWRITE:
 * 1. [DONE] Make binary format agnostic
 * 2. [DONE] Figure out the proper way to load the file (i.e. explicitly allocate the `struct file` or do it silently like it's currently done)
 * 3. [DONE] Probably move out the user stack setup -> No
 * 4. Refactor the user stack setup to avoid hardcoded values
 * 5. [DONE] Think about heap VMA. Do we still need the `brk` pointer? -> Yes, for brk(2) and sbrk(2) syscalls
 * 6. [DONE] Do we really need to return a pointer to a user stack? -> Yes
 * 7. [DONE] Is it really a way to `free_user_image`? -> Yes
 * 8. [DONE] How to properly allocate pages? Do we need to perform two explicit steps: physical allocation and virtual mapping? -> Yes
 * 9. [DONE] Update the page fault hander for the new VMA struct -> No, that requires lazy allocation strategy
 */
i32 elf_load_binary(struct binparams *bprm, struct registers_state *regs) {
	u32 last_addr = 0;
	i32 retval = -1;

	struct elf_header *hdr = (struct elf_header *)bprm->buffer;
	if (!is_elf(hdr)) {
		return -ENOEXEC;
	}

	//dump_elf_header(hdr);

	if (hdr->type != ET_EXEC || hdr->machine != EM_X86) {
		return -ENOEXEC;
	}

	free_user_image();

	for (i32 i = 0; i < hdr->ph_num; ++i) {
		struct elf_program_header phdr;
		const i32 offset = hdr->phoff + hdr->ph_size * i;
		retval = read_exec_buffer(bprm->inode, (i8 *)&phdr, sizeof(phdr), offset);
		if (retval < 0) {
			return retval;
		}

		//debug("ELF program header:\r\n");
		//dump_program_header(phdr);

		if (phdr.type != PT_LOAD) {
			debug("ELF program header is not PT_LOAD: %d\r\n", phdr.type);
			continue;
		}

		if (phdr.memsz == 0) {
			continue;
		}

		u32 blocks = phdr.memsz / PMM_BLOCK_SIZE;
		if (phdr.memsz % PMM_BLOCK_SIZE != 0) {
			++blocks;
		}
		void *code_phys_frames = allocate_blocks(blocks);
		if (code_phys_frames == NULL) {
		    panic("Failed to allocate new physical block: not enough space :(");
		    return -ENOMEM;
		}

		u32 flags = PAGING_FLAG_PRESENT | PAGING_FLAG_USER;
		flags |= (phdr.flags & PF_W) ? PAGING_FLAG_WRITEABLE : 0;
		/* Map necessary pages for code */
		for (u32 j = 0, addr = phdr.vaddr; j < blocks; ++j, addr += 0x1000) {
			u32 phys_code_page = (u32)code_phys_frames + j * 0x1000;
			map_page(phys_code_page, addr, flags);
		}

		retval = read_exec_buffer(bprm->inode, (i8 *)phdr.vaddr, phdr.filesz, phdr.offset);
		if (retval < 0) {
			return retval;
		}
		memset((void *)(phdr.vaddr + phdr.filesz), 0, phdr.memsz - phdr.filesz);
		last_addr = phdr.vaddr + phdr.memsz;
	}

	i8 *usp = setup_user_stack(bprm->argc, bprm->argv, bprm->envc, bprm->envp);
	if (!usp) {
		return -ENOMEM;
	}
	if (setup_heap(last_addr)) {
		return -ENOMEM;
	}

	regs->eip = hdr->entry;
	regs->useresp = (u32)usp;

	return 0;
}

static void dump_elf_header(struct elf_header *elf_h) {
	debug("magic_number - %x\r\n", elf_h->magic_number);
	debug("elf_ascii[3] - %c, %c, %c\r\n", elf_h->elf_ascii[0], elf_h->elf_ascii[1], elf_h->elf_ascii[2]);
	debug("class - %x\r\n", elf_h->class);
	debug("data - %x\r\n", elf_h->data);
	debug("version - %x\r\n", elf_h->version);
	debug("os_abi - %x\r\n", elf_h->os_abi);
	debug("padding[8] - [0]....8 times\r\n");
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
