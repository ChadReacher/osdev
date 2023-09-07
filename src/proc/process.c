#include <process.h>
#include <heap.h>
#include <isr.h>
#include <pmm.h>
#include <tss.h>
#include <stdio.h>
#include <timer.h>
#include <debug.h>
#include <panic.h>
#include <scheduler.h>
#include <string.h>

extern void irq_ret();
extern process_t *proc_list;
extern process_t *current_process;

u32 next_pid = 1;

void userinit() {
	vfs_node_t *vfs_node = vfs_get_node("/bin/init");
	if (!vfs_node) {
		PANIC("Failed to start 'init' process\r\n");
	}
	u32 *data = malloc(vfs_node->length);
	memset((i8 *)data, 0, vfs_node->length);
	vfs_read(vfs_node, 0, vfs_node->length, (i8 *)data);
	elf_load(data);
	free(data);

	current_process = proc_list;
	current_process->parent = current_process;
}

process_t *proc_alloc() {
	process_t *process = malloc(sizeof(process_t));
	memset(process, 0, sizeof(process_t));

	process->pid = next_pid++;
	process->state = RUNNABLE;
	process->next = NULL;
	process->fds = malloc(FDS_NUM * sizeof(file));
	memset(process->fds, 0, FDS_NUM * sizeof(file));
	process->kernel_stack_bottom = malloc(4096);
	memset(process->kernel_stack_bottom, 0, 4096);
	u32 *sp = (u32 *)ALIGN_DOWN((u32)process->kernel_stack_bottom + 4096 - 1, 4);

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
	process->regs = (registers_state *)(sp + 1);
	*sp-- = (u32)irq_ret;	// irq_ret eip (to return back to the end of the interrupt routine)
	*sp-- = 0x0;			// ebp
	*sp-- = 0x0; 			// ebx
	*sp-- = 0x0; 			// esi
	*sp-- = 0x0; 			// edi
	++sp;

	process->kernel_stack_top = (void *)sp;
	process->context = (context_t *)sp;

	add_process_to_list(process);

	return process;
}

/*
void process_free(process_t *proc) {
	free(proc->kernel_stack_bottom);

	void *page_dir_phys = (void *)proc->directory;
	map_page(page_dir_phys, 0xE0000000);
	page_directory_t *page_dir = (page_directory_t *)0xE0000000;
	for (u32 i = 0; i < 768; ++i) {
		if (!page_dir->entries[i]) {
			continue;
		}
		page_directory_entry pde = page_dir->entries[i];
		void *table_phys = (void *)GET_FRAME(pde);
		map_page(table_phys, 0xEA000000); // Temporary mapping
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
	
	for (u32 i = 0; i < 32; ++i) {
		file *f = &proc->fds[i];
		if (f->vfs_node && f->vfs_node != (void *) -1) {
			vfs_close(f->vfs_node);
			memset(f, 0, sizeof(file));
		}
	}
	free(proc->fds);
	free(proc->cwd);
	free(proc);
}
*/

i32 proc_get_fd(process_t *proc) {
	for (u32 i = 3; i < 32; ++i) {
		if (!proc->fds[i].used) {
			proc->fds[i].used = true;
			return i;
		}
	}
	return -1;
}
