#include <stdio.h>
#include <unistd.h>
#include <stdlib.h>
#include <dirent.h>
#include <fcntl.h>

int main(i32 argc, i8 *argv[], i8 *envp[]) {
	printf("argc - %x\n", argc);
	for (i32 i = 0; i < argc; ++i) {
		printf("argv[%d] - %s\n", i, argv[i]);
	}
	DIR *dirp = opendir("/");
	struct dirent *dent = readdir(dirp);
	printf("name - %s, inode - %d\n", dent->name, dent->inode);
	free(dent);
	closedir(dirp);

	//printf("pid - %d\n", getpid());
	//putchar('h');
	//putchar('i');
	//putchar('\n');
	//u8 m = getchar();
	//putchar(m);
	//printf("Test malloc:\n");
	//u8 *x = (u8 *)malloc(4);
	//x[0] = 1;
	//x[1] = 2;
	//x[2] = 3;
	//x[3] = 4;
	//for (u32 i = 0; i < 4; ++i) {
	//	printf("%d\n", x[i]);
	//}
	//printf("Before free\n");
	//free(x);
	//for (u32 i = 0; i < 10; ++i) {
	//	printf("friends\n");
	//}
	return 123;
}
