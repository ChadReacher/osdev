#include <types.h>
#include <serial.h>
#include <debug.h>
#include <isr.h>
#include <syscall.h>
#include <timer.h>
#include <keyboard.h>
#include <cmos.h>
#include <pmm.h>
#include <paging.h>
#include <screen.h>
#include <heap.h>
#include <pci.h>
#include <vfs.h>
#include <ata.h>
#include <ext2.h>
#include <elf.h>
//#include <kshell.h>
#include <string.h>
//#include <list.h>
//#include <generic_tree.h>

void _start() {
	serial_init();

	isr_init();
	syscall_init();
	irq_init();
	timer_init(50);
	keyboard_init();
	cmos_rtc_init();
	pmm_init();
	paging_init();
	screen_clear();
	heap_init();
	pci_init();
	vfs_init();
	ata_init();
	ext2_init("/dev/hdb", "/");

	kprintf("/----------------------------------------------\\\n");
	kprintf("\t\tWelcome to the OS.\n");
	kprintf("\\----------------------------------------------/\n");

	vfs_print();
	
	/*
	u32 ret_fd, sz, have_read, have_written;
	i8 *buff;

	DEBUG("%s", "Create file 'sky' with open syscall(O_CREAT flag)\r\n");
	ret_fd = open("/sky", O_CREAT, 0);
	close(ret_fd);

	DEBUG("%s", "TESTING WRITE SYSCALL\r\n");

	DEBUG("%s", "O_WRONLY for stdin\r\n");
	sz = 2;
	buff = malloc(sz + 1);
	memset(buff, 0, sz + 1);
	buff[0] = 'c';
	buff[0] = 'd';
	have_written = write(0, buff, sz);
	DEBUG("Have written - %d\r\n", have_written);
	free(buff);

	DEBUG("%s", "O_WRONLY for stdout\r\n");
	sz = 2;
	buff = malloc(sz + 1);
	memset(buff, 0, sz + 1);
	buff[0] = 'c';
	buff[1] = 'd';
	have_written = write(1, buff, sz);
	DEBUG("Have written - %d\r\n", have_written);
	free(buff);

	DEBUG("%s", "O_WRONLY for stderr\r\n");
	sz = 2;
	buff = malloc(sz + 1);
	memset(buff, 0, sz + 1);
	buff[0] = 'c';
	buff[1] = 'd';
	have_written = write(2, buff, sz);
	DEBUG("Have written - %d\r\n", have_written);
	free(buff);

	DEBUG("%s", "O_RDONLY\r\n");
	ret_fd = open("/sky", O_RDONLY, 0);
	DEBUG("Got ret_fd - %d\r\n", ret_fd);
	sz = 2;
	buff = malloc(sz + 1);
	memset(buff, 0, sz + 1);
	buff[0] = 'c';
	buff[1] = 'd';
	have_written = write(ret_fd, buff, sz);
	DEBUG("Have written - %d\r\n", have_written);
	free(buff);
	close(ret_fd);

	DEBUG("%s", "O_WRONLY\r\n");
	ret_fd = open("/sky", O_WRONLY, 0);
	DEBUG("Got ret_fd - %d\r\n", ret_fd);
	sz = 2;
	buff = malloc(sz + 1);
	memset(buff, 0, sz + 1);
	buff[0] = 'c';
	buff[1] = 'd';
	have_written = write(ret_fd, buff, sz);
	DEBUG("Have written - %d\r\n", have_written);
	free(buff);
	close(ret_fd);

	DEBUG("%s", "Read written data\r\n");
	ret_fd = open("/sky", O_RDONLY, 0);
	DEBUG("Got ret_fd - %d\r\n", ret_fd);
	sz = 2;
	buff = malloc(sz + 1);
	memset(buff, 0, sz + 1);
	have_read = read(ret_fd, buff, sz);
	DEBUG("Have read - %d, %s\r\n", have_read, buff);
	free(buff);
	close(ret_fd);

	
	DEBUG("%s", "O_RDWR\r\n");
	ret_fd = open("/sky", O_RDWR, 0);
	DEBUG("Got ret_fd - %d\r\n", ret_fd);
	sz = 2;
	buff = malloc(sz + 1);
	memset(buff, 0, sz + 1);
	buff[0] = 'c';
	buff[1] = 'd';
	have_written = write(ret_fd, buff, sz);
	DEBUG("Have written - %d\r\n", have_written);
	free(buff);
	close(ret_fd);

	DEBUG("%s", "Read written data\r\n");
	ret_fd = open("/sky", O_RDONLY, 0);
	DEBUG("Got ret_fd - %d\r\n", ret_fd);
	sz = 2;
	buff = malloc(sz + 1);
	memset(buff, 0, sz + 1);
	have_read = read(ret_fd, buff, sz);
	DEBUG("Have read - %d, %s\r\n", have_read, buff);
	free(buff);
	close(ret_fd);

	DEBUG("%s", "O_APPEND\r\n");
	ret_fd = open("/sky", O_APPEND, 0);
	DEBUG("Got ret_fd - %d\r\n", ret_fd);
	sz = 2;
	buff = malloc(sz + 1);
	memset(buff, 0, sz + 1);
	buff[0] = 'c';
	buff[1] = 'd';
	have_written = write(ret_fd, buff, sz);
	DEBUG("Have written - %d\r\n", have_written);
	free(buff);
	close(ret_fd);

	DEBUG("%s", "O_CREAT\r\n");
	ret_fd = open("/sky", O_CREAT, 0);
	DEBUG("Got ret_fd - %d\r\n", ret_fd);
	sz = 2;
	buff = malloc(sz + 1);
	memset(buff, 0, sz + 1);
	buff[0] = 'c';
	buff[1] = 'd';
	have_written = write(ret_fd, buff, sz);
	DEBUG("Have written - %d\r\n", have_written);
	free(buff);
	close(ret_fd);

	DEBUG("%s", "O_TRUNC\r\n");
	ret_fd = open("/sky", O_TRUNC, 0);
	DEBUG("Got ret_fd - %d\r\n", ret_fd);
	sz = 2;
	buff = malloc(sz + 1);
	memset(buff, 0, sz + 1);
	buff[0] = 'c';
	buff[1] = 'd';
	have_written = write(ret_fd, buff, sz);
	DEBUG("Have written - %d\r\n", have_written);
	free(buff);
	close(ret_fd);

	DEBUG("%s", "O_WRONLY serveral times\r\n");
	ret_fd = open("/sky", O_WRONLY, 0);
	DEBUG("Got ret_fd - %d\r\n", ret_fd);

	sz = 2;
	buff = malloc(sz + 1);
	memset(buff, 0, sz + 1);
	buff[0] = 'c';
	buff[1] = 'd';
	have_written = write(ret_fd, buff, sz);
	DEBUG("Have written - %d\r\n", have_written);

	buff[0] = 'a';
	buff[1] = 'b';
	have_written = write(ret_fd, buff, sz);
	DEBUG("Have written - %d\r\n", have_written);

	buff[0] = '!';
	buff[1] = '!';
	have_written = write(ret_fd, buff, sz);
	DEBUG("Have written - %d\r\n", have_written);

	free(buff);
	close(ret_fd);

	DEBUG("%s", "Read written data\r\n");
	ret_fd = open("/sky", O_RDONLY, 0);
	DEBUG("Got ret_fd - %d\r\n", ret_fd);
	sz = 2;
	buff = malloc(sz + 1);
	memset(buff, 0, sz + 1);
	have_read = read(ret_fd, buff, sz);
	DEBUG("Have read - %d, %s\r\n", have_read, buff);
	free(buff);
	close(ret_fd);

	DEBUG("%s", "O_WRONLY overwrites\r\n");
	ret_fd = open("/sky", O_WRONLY, 0);
	DEBUG("Got ret_fd - %d\r\n", ret_fd);

	sz = 2;
	buff = malloc(sz + 1);
	memset(buff, 0, sz + 1);
	buff[0] = 'C';
	buff[1] = 'D';
	have_written = write(ret_fd, buff, sz);
	DEBUG("Have written - %d\r\n", have_written);
	free(buff);
	close(ret_fd);

	DEBUG("%s", "Read written data\r\n");
	ret_fd = open("/sky", O_RDONLY, 0);
	DEBUG("Got ret_fd - %d\r\n", ret_fd);
	sz = 2;
	buff = malloc(sz + 1);
	memset(buff, 0, sz + 1);
	have_read = read(ret_fd, buff, sz);
	DEBUG("Have read - %d, %s\r\n", have_read, buff);
	free(buff);
	close(ret_fd);


	DEBUG("%s", "O_WRONLY with O_APPEND\r\n");
	ret_fd = open("/sky", O_WRONLY | O_APPEND, 0);
	DEBUG("Got ret_fd - %d\r\n", ret_fd);

	sz = 2;
	buff = malloc(sz + 1);
	memset(buff, 0, sz + 1);
	buff[0] = '?';
	buff[1] = '?';
	have_written = write(ret_fd, buff, sz);
	DEBUG("Have written - %d\r\n", have_written);
	free(buff);
	close(ret_fd);

	DEBUG("%s", "Read written data\r\n");

	ret_fd = open("/sky", O_RDONLY, 0);
	DEBUG("Got ret_fd - %d\r\n", ret_fd);
	sz = 2;
	buff = malloc(sz + 1);
	memset(buff, 0, sz + 1);
	have_read = read(ret_fd, buff, sz);
	DEBUG("Have read - %d, %s\r\n", have_read, buff);
	free(buff);
	close(ret_fd);

	DEBUG("%s", "TESTING LSEEK SYSCALL\r\n");

	ret_fd = open("/sky", O_RDONLY, 0);
	DEBUG("Got ret_fd - %d\r\n", ret_fd);
	sz = 2;
	buff = malloc(sz + 1);
	memset(buff, 0, sz + 1);

	DEBUG("%s", "SEEK_SET with 2\r\n");
	lseek(ret_fd, 2, SEEK_SET);	
	have_read = read(ret_fd, buff, sz);
	DEBUG("Have read - %d, %s\r\n", have_read, buff);

	DEBUG("%s", "SEEK_CUR with 2\r\n");
	lseek(ret_fd, 2, SEEK_CUR);	
	have_read = read(ret_fd, buff, sz);
	DEBUG("Have read - %d, %s\r\n", have_read, buff);

	DEBUG("%s", "SEET_SET with 0\r\n");
	lseek(ret_fd, 0, SEEK_SET);	
	have_read = read(ret_fd, buff, sz);
	DEBUG("Have read - %d, %s\r\n", have_read, buff);

	DEBUG("%s", "SEET_END with 5\r\n");
	lseek(ret_fd, 5, SEEK_END);	
	have_read = read(ret_fd, buff, sz);
	DEBUG("Have read - %d, %s\r\n", have_read, buff);

	free(buff);
	close(ret_fd);


	DEBUG("%s", "TESTING UNLINK SYSCALL\r\n");
	DEBUG("%s", "Try to delete /sky\r\n");
	i32 ret_unlink = unlink("/sky");
	DEBUG("ret_unlink - %d\r\n", ret_unlink);

	DEBUG("%s", "Try to delete /soil\r\n");
	ret_unlink = unlink("/soil");
	DEBUG("ret_unlink - %d\r\n", ret_unlink);


	*/
	vfs_node_t *vfs_node = vfs_get_node("/bin/init");
	u32 *data = malloc(vfs_node->length);
	memset((i8 *)data, 0, vfs_node->length);
	vfs_read(vfs_node, 0, vfs_node->length, (i8 *)data);
	//u32 init_fd = open("/bin/init", O_RDONLY, 0);
	//read(init_fd, (i8 *)data, vfs_node->length);
	elf_header_t *elf = elf_load(data);

	if (elf) {
		DEBUG("Loaded elf entry at 0x%p\r\n", elf->entry);
		typedef int callable(void);
		callable *c = (callable *)(elf->entry);
		i32 res = c();
		DEBUG("Return code - %d\r\n", res);
		elf_unload(elf);
		free(elf);
	}

	/*
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
	strcat(str_to_cat, another_str_to_cat);
	DEBUG("Result of strcat - %s\r\n", str_to_cat);
	free(str_to_cat);

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
	*/

	//kshell();	
}
