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
#include "pci.h"
#include "ata.h"
#include "ext2.h"
#include "memory.h"

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
	pci_init();
	vfs_init();
	ata_init();
	ext2_init("/dev/hdb", "/");

	DEBUG("%s", "-------------------------\r\n");
	vfs_print();
	DEBUG("%s", "-------------------------\r\n");

	//DEBUG("%s", "-------------------------\r\n");
	//vfs_node_t *got_vfs_node = vfs_get_node("/dev/hdb");
	//DEBUG("node name - %s\r\n", got_vfs_node->name);
	//DEBUG("%s", "-------------------------\r\n");

	//DEBUG("%s", "-------------------------\r\n");
	//got_vfs_node = vfs_get_node("/dev/sdb");
	//DEBUG("node is %p\r\n", got_vfs_node);
	//DEBUG("%s", "-------------------------\r\n");	

	//DEBUG("%s", "-------------------------\r\n");
	//got_vfs_node = vfs_get_node("/bin/proc");
	//DEBUG("node is %p\r\n", got_vfs_node);
	//DEBUG("%s", "-------------------------\r\n");	

	/*
	DEBUG("%s", "-------------------------\r\n");
	i8 *filebuf = malloc(5);
	memset(filebuf, 0, 5);
	u8 have_read = vfs_read(vfs_get_node("/hi.txt"), 17, 5, filebuf);
	DEBUG("Have read - %d\r\n", have_read);
	DEBUG("%s", "File buf: \r\n");
	for (u32 i = 0; i < 5; ++i) {
		DEBUG("%x\r\n", filebuf[i]);
	}
	//DEBUG("%s\r\n", filebuf);
	free(filebuf);
	DEBUG("%s", "-------------------------\r\n");	
	*/

	/*
	DEBUG("%s", "-------------------------\r\n");	
	ext2_inode_table *inode = ext2_get_inode_table(12);
	DEBUG("Before, size = %x\r\n", inode->size);
	i8 *test = "testing message.";
	strlen(test);
	DEBUG("strlen(test) = %x\r\n", strlen(test));
	ext2_write_inode_filedata(inode, 12, 3, strlen(test), test);
	inode = ext2_get_inode_table(12);
	DEBUG("After, size = %x\r\n", inode->size);
	DEBUG("%s", "-------------------------\r\n");	
	*/


	//DEBUG("%s", "-------------------------\r\n");
	//DEBUG("%s", "Trying to find a file 'hi.txt'\r\n");
	//vfs_node_t *file_node = ext2_finddir(vfs_root, "hi.txt");
	//if (file_node != NULL) {
	//	DEBUG("Found a vfs node - %s\r\n", file_node->name);
	//	DEBUG("Mask - 0x%x\r\n", file_node->mask);
	//	DEBUG("Flags - 0x%x\r\n", file_node->flags);
	//	DEBUG("UID - 0x%x\r\n", file_node->uid);
	//	DEBUG("GID - 0x%x\r\n", file_node->gid);
	//	DEBUG("Inode - 0x%x\r\n", file_node->inode);
	//	DEBUG("Length - 0x%x\r\n", file_node->length);
	//}
	//DEBUG("%s", "-------------------------\r\n");

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

	DEBUG("%s", "Testing strcat\r\n");
	i8 *str_to_cat = malloc(7);
	memset(str_to_cat, 0, 7);
	memcpy(str_to_cat, "cat", 3);
	i8 *another_str_to_cat = "man";
	DEBUG("First str - %s, second str - %s\r\n", str_to_cat, another_str_to_cat);
	i8 *concatenated_str = strcat(str_to_cat, another_str_to_cat);
	DEBUG("Result of strcat - %s\r\n", concatenated_str);

	i8 *complicated_path = "/usr/data/bin";
	DEBUG("Canonilize path - %s\r\n", complicated_path);
	DEBUG("Result - %s\r\n", canonilize_path(complicated_path));

	complicated_path = "/usr/data/./bin";
	DEBUG("Canonilize path - %s\r\n", complicated_path);
	DEBUG("Result - %s\r\n", canonilize_path(complicated_path));

	complicated_path = "/usr/data/..";
	DEBUG("Canonilize path - %s\r\n", complicated_path);
	DEBUG("Result - %s\r\n", canonilize_path(complicated_path));

	complicated_path = "/usr/..";
	DEBUG("Canonilize path - %s\r\n", complicated_path);
	DEBUG("Result - %s\r\n", canonilize_path(complicated_path));

	complicated_path = "/..";
	DEBUG("Canonilize path - %s\r\n", complicated_path);
	DEBUG("Result - %s\r\n", canonilize_path(complicated_path));

	u32 virt_a = 0xC0010000;
	u32 phys_a = (u32)virtual_to_physical((void *)virt_a);
	DEBUG("Translating virtual address(%x) to physical address(%x)\r\n", virt_a, phys_a);

	kshell();	
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
