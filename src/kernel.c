#include "types.h"
#include "isr.h"
#include "timer.h"
#include "keyboard.h"
#include "screen.h"
#include "stdio.h"
#include "serial.h"
#include "debug.h"
#include "bios_memory_map.h"
#include "pmm.h"
#include "paging.h"
#include "cmos.h"
#include "kshell.h"
#include "syscall.h"
#include "heap.h"
#include "list.h"
#include "generic_tree.h"
#include "string.h"
#include "vfs.h"

void print_physical_memory_info();

void _start() {
	serial_init();
	DEBUG("%s", "OS has started\r\n");

	isr_init();
	syscall_init();
	irq_init();
	timer_init(50);
	keyboard_init();
	cmos_rtc_init();
	pmm_init();
	paging_init();
	screen_clear();
	print_physical_memory_info();
	heap_init();



	u8 *p = (u8 *)malloc(5);
	DEBUG("Allocated 5 bytes at %p\r\n", p);
	for (u8 i = 1; i < 6; ++i) {
		p[i - 1] = i;
	}
	for (u8 i = 0; i < 5; ++i) {
		DEBUG("%d\r\n", p[i]);
	}
	DEBUG("%s", "\r\n");
	p = (u8 *)realloc(p, 15);
	DEBUG("After REALLOCATING TO 15 bytes at %p\r\n", p);
	for (u8 i = 0; i < 15; ++i) {
		DEBUG("%d\r\n", p[i]);
	}
	DEBUG("%s", "\r\n");
	for (u8 i = 5; i < 16; ++i) {
		p[i] = i;
	}
	for (u8 i = 0; i < 15; ++i) {
		DEBUG("%d\r\n", p[i]);
	}
	DEBUG("%s", "\r\n");
	free((void*)p);
	DEBUG("%s", "Freed 5 bytes with p\r\n");



	list_t *list = list_create();
	list_insert_front(list, (void*)5);
	list_insert_front(list, (void*)1);
	list_insert_front(list, (void*)2);
	list_insert_back(list, (void*)4);
	list_insert_front(list, (void*)8);
	list_insert_back(list, (void*)9);
	list_node_t *node = list->head;
	DEBUG("Head - %p, head->val = %d\r\n", list->head, list->head->val);
	DEBUG("Tail - %p, tail->val = %d\r\n", list->tail, list->tail->val);
	while (node) {
		DEBUG("%d\r\n", (int)node->val);
		node = node->next;
	}
	list_destroy(list);



	DEBUG("%s", "Testing generic tree with integers\r\n");
	tree_t *tree = generic_tree_create();
	tree_node_t *node_root = malloc(sizeof(tree_node_t));
	node_root->val = (void*)1;
	node_root->children = list_create();
	node_root->parent = NULL;
	tree->root = node_root;
	tree_node_t *node2 = generic_tree_insert_at(tree, tree->root, (void*)2);
	tree_node_t *node3 = generic_tree_insert_at(tree, tree->root, (void*)3);
	generic_tree_insert_at(tree, node2, (void*)4);
	tree_node_t *node5 = generic_tree_insert_at(tree, node2, (void*)5);
	generic_tree_insert_at(tree, node3, (void*)6);
	generic_tree_insert_at(tree, node3, (void*)8);
	generic_tree_insert_at(tree, node5, (void*)7);
	generic_tree_print(tree);
	generic_tree_destroy(tree);

	DEBUG("%s", "Testing strcspn(\"fcba73\", \"1234567890\")\r\n");
	int i = strcspn("fcba73", "1234567890");
	DEBUG("The first number in str is at position %d, index %d\r\n", i + 1, i);

	DEBUG("%s", "Testing strsep(\"home/documents/folder1\", \"/\")\r\n");
	i8 *str = "home/documents/folder1";
	i8 *ret;
	do {
		ret = strsep(&str, "/");
		DEBUG("First string before delimeter - %s\r\n", ret);
		DEBUG("The rest of the initial string - %s\r\n", str);
	} while (str);

	DEBUG("hello == hello? %d\r\n", strcmp("hello", "hello"));
	DEBUG("%s == %s? %d\r\n", "hello", "Hello", strcmp("hello", "Hello"));
	DEBUG("%s == %s? %d\r\n", "", "hello", strcmp("", "hello"));
	DEBUG("%s == %s? %d\r\n", "", "", strcmp("", ""));

	DEBUG("%s", "Testing strncpy with normal parameters:\r\n");
	i8 *src = "trying";
	i8 *dest = (i8 *)malloc(4);
	DEBUG("Before src = %s\r\n", src);
	strncpy(dest, src, 3);
	dest[3] = '\0';
	DEBUG("After src = %s, dest = %s\r\n", src, dest);
	free(dest);

	DEBUG("%s", "Testing strncpy with normal parameters:\r\n");
	src = "try";
	dest = (i8 *)malloc(4);
	DEBUG("Before src = %s\r\n", src);
	strncpy(dest, src, 4);
	DEBUG("After src = %s, dest = %s\r\n", src, dest);
	free(dest);

	DEBUG("%s", "Testing strncpy when n > len of src:\r\n");
	src = "trying";
	dest = (i8 *)malloc(8);
	DEBUG("Before src = %s\r\n", src);
	strncpy(dest, src, 8);
	DEBUG("After src = %s, dest = %s\r\n", src, dest);
	free(dest);

	DEBUG("%s", "Testing strdup\r\n");
	i8 *str_to_dup = "trying123";
	i8 *rett = strdup(str_to_dup);
	DEBUG("Init - %s, ret - %s\r\n", str_to_dup, rett);
	DEBUG("Init - %p, ret - %p\r\n", str_to_dup, rett);
	free(rett);

	vfs_init();

	vfs_node_t *vfs_node_to_mount = malloc(sizeof(vfs_node_t));
	strcpy(vfs_node_to_mount->name, "home");
	vfs_node_to_mount->mask = 0; 
	vfs_node_to_mount->uid = 0;
	vfs_node_to_mount->gid = 0;
	vfs_node_to_mount->inode = 0;
	vfs_node_to_mount->length = 0;
	vfs_node_to_mount->impl = 0;
	vfs_node_to_mount->read = NULL;
	vfs_node_to_mount->write = NULL;
	vfs_node_to_mount->open = NULL; 
	vfs_node_to_mount->close = NULL; 
	vfs_node_to_mount->readdir = NULL;
	vfs_node_to_mount->finddir = NULL;
	vfs_node_to_mount->ptr = NULL;
	vfs_node_to_mount->flags = 0;

	DEBUG("%s", "===============\r\n");
	DEBUG("%s", "Mounting /home\r\n");
	vfs_mount("/home", vfs_node_to_mount);
	DEBUG("%s", "===============\r\n");

	vfs_node_t *vfs_node_to_mount2 = malloc(sizeof(vfs_node_t));
	strcpy(vfs_node_to_mount2->name, "bin");
	vfs_node_to_mount2->mask = 0; 
	vfs_node_to_mount2->uid = 0;
	vfs_node_to_mount2->gid = 0;
	vfs_node_to_mount2->inode = 0;
	vfs_node_to_mount2->length = 0;
	vfs_node_to_mount2->impl = 0;
	vfs_node_to_mount2->read = NULL;
	vfs_node_to_mount2->write = NULL;
	vfs_node_to_mount2->open = NULL; 
	vfs_node_to_mount2->close = NULL; 
	vfs_node_to_mount2->readdir = NULL;
	vfs_node_to_mount2->finddir = NULL;
	vfs_node_to_mount2->ptr = NULL;
	vfs_node_to_mount2->flags = 0;
	DEBUG("%s", "===============\r\n");
	DEBUG("%s", "Mounting /home/bin\r\n");
	vfs_mount("/home/bin", vfs_node_to_mount2);
	DEBUG("%s", "===============\r\n");

	vfs_node_t *vfs_node_to_mount3 = malloc(sizeof(vfs_node_t));
	vfs_node_to_mount3->mask = 0; 
	vfs_node_to_mount3->uid = 0;
	vfs_node_to_mount3->gid = 0;
	vfs_node_to_mount3->inode = 0;
	vfs_node_to_mount3->length = 0;
	vfs_node_to_mount3->impl = 0;
	vfs_node_to_mount3->read = NULL;
	vfs_node_to_mount3->write = NULL;
	vfs_node_to_mount3->open = NULL; 
	vfs_node_to_mount3->close = NULL; 
	vfs_node_to_mount3->readdir = NULL;
	vfs_node_to_mount3->finddir = NULL;
	vfs_node_to_mount3->ptr = NULL;
	vfs_node_to_mount3->flags = 0;
	DEBUG("%s", "===============\r\n");
	DEBUG("%s", "Mounting /sbin\r\n");
	strcpy(vfs_node_to_mount3->name, "sbin");
	vfs_mount("/sbin", vfs_node_to_mount3);
	DEBUG("%s", "===============\r\n");

	vfs_node_t *vfs_node_to_mount4 = malloc(sizeof(vfs_node_t));
	strcpy(vfs_node_to_mount4->name, "games");
	vfs_node_to_mount4->mask = 0; 
	vfs_node_to_mount4->uid = 0;
	vfs_node_to_mount4->gid = 0;
	vfs_node_to_mount4->inode = 0;
	vfs_node_to_mount4->length = 0;
	vfs_node_to_mount4->impl = 0;
	vfs_node_to_mount4->read = NULL;
	vfs_node_to_mount4->write = NULL;
	vfs_node_to_mount4->open = NULL; 
	vfs_node_to_mount4->close = NULL; 
	vfs_node_to_mount4->readdir = NULL;
	vfs_node_to_mount4->finddir = NULL;
	vfs_node_to_mount4->ptr = NULL;
	vfs_node_to_mount4->flags = 0;
	DEBUG("%s", "===============\r\n");
	DEBUG("%s", "Mounting /home/games\r\n");
	vfs_mount("/home/games", vfs_node_to_mount4);
	DEBUG("%s", "===============\r\n");

	vfs_node_t *vfs_node_to_mount5 = malloc(sizeof(vfs_node_t));
	strcpy(vfs_node_to_mount5->name, "games");
	vfs_node_to_mount5->mask = 0; 
	vfs_node_to_mount5->uid = 0;
	vfs_node_to_mount5->gid = 0;
	vfs_node_to_mount5->inode = 0;
	vfs_node_to_mount5->length = 0;
	vfs_node_to_mount5->impl = 0;
	vfs_node_to_mount5->read = NULL;
	vfs_node_to_mount5->write = NULL;
	vfs_node_to_mount5->open = NULL; 
	vfs_node_to_mount5->close = NULL; 
	vfs_node_to_mount5->readdir = NULL;
	vfs_node_to_mount5->finddir = NULL;
	vfs_node_to_mount5->ptr = NULL;
	vfs_node_to_mount5->flags = 0;
	DEBUG("%s", "===============\r\n");
	DEBUG("%s", "Mounting /home/games\r\n");
	vfs_mount("/home/games", vfs_node_to_mount5);
	DEBUG("%s", "===============\r\n");

	vfs_node_t *vfs_node_to_mount6 = malloc(sizeof(vfs_node_t));
	strcpy(vfs_node_to_mount6->name, "games");
	vfs_node_to_mount6->mask = 0; 
	vfs_node_to_mount6->uid = 0;
	vfs_node_to_mount6->gid = 0;
	vfs_node_to_mount6->inode = 0;
	vfs_node_to_mount6->length = 0;
	vfs_node_to_mount6->impl = 0;
	vfs_node_to_mount6->read = NULL;
	vfs_node_to_mount6->write = NULL;
	vfs_node_to_mount6->open = NULL; 
	vfs_node_to_mount6->close = NULL; 
	vfs_node_to_mount6->readdir = NULL;
	vfs_node_to_mount6->finddir = NULL;
	vfs_node_to_mount6->ptr = NULL;
	vfs_node_to_mount6->flags = 0;
	DEBUG("%s", "===============\r\n");
	DEBUG("%s", "Mounting /home/games/arcade/fast\r\n");
	vfs_mount("/home/games/arcade/fast", vfs_node_to_mount6);
	DEBUG("%s", "===============\r\n");

	DEBUG("%s", "-------------------------\r\n");
	vfs_print();
	DEBUG("%s", "-------------------------\r\n");

	kprintf(PROMPT);
	for (;;) {
		kshell(keyboard_get_last_scancode());
	}
}

void print_physical_memory_info() {
	memory_map_entry *mmap_entry;
	u32 num_entries;

	mmap_entry = (memory_map_entry *)BIOS_MEMORY_MAP;
	num_entries = *((u32 *)BIOS_NUM_ENTRIES);

	kprintf("Physical memory info: ");
	kprintf("Total number of entries: %d\n", num_entries);
	for (u8 i = 0; i < num_entries; ++i) {
		kprintf("Region: %x | Base: %x | Length: %x | Type(%d): ", i, (u32)mmap_entry->base_address, (u32)mmap_entry->length, mmap_entry->type);
		switch (mmap_entry->type) {
			case 1:
				kprintf("Available Memory");
				break;
			case 2:
				kprintf("Reserved Memory");
				break;
			case 3:
				kprintf("ACPI Reclaim Memory");
				break;
			case 4:
				kprintf("ACPI NVS Memory");
				break;
			default:
				kprintf("Undefined Memory");
				break;
		}
		kprintf("\n");
		++mmap_entry; 
	}
	kprintf("\n");
	--mmap_entry;
	kprintf("Total amount of memory(in bytes): %x\n", (u32)mmap_entry->base_address + (u32)mmap_entry->length - 1);
}
