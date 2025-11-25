#include <stdio.h>
#include <unistd.h>
#include <sys/mount.h>

int main(int argc, char *argv[]) {
    if (argc != 3) {
		printf("usage: %s <DEVICE> <DESTINATION>\n", argv[0]);
        return 1;
    }
    i32 err = mount(argv[1], argv[2]);
    if (err) {
        perror("mount failed");
        return 1;
    }
    return 0;
}
