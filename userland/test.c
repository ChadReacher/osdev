#include <stdio.h>
#include <unistd.h>
#include <dirent.h>
#include <fcntl.h>

int main(i32 argc, i8 *argv[]) {
    i32 i;
    printf("argc - %x\n", argc);
    for (i = 0; i < argc; ++i) {
        printf("argv[%d] - %s\n", i, argv[i]);
    }
    for (i = 0; i < 30; ++i) {
        printf("%d, ", i);
        i32 slept = sleep(3);
        printf("slept for %d\n", slept);
    }
    return 123;
}
