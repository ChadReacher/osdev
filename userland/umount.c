#include <stdio.h>
#include <unistd.h>
#include <sys/mount.h>

int main(int argc, char *argv[]) {
    if (argc != 2) {
		printf("usage: %s <TARGET>\n", argv[0]);
        return 1;
    }
    i32 err = umount(argv[1]);
    if (err) {
        perror("mount failed");
        return 1;
    }
    return 0;
}
