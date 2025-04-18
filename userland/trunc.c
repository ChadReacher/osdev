#include <stdio.h>
#include <unistd.h>

int main(int argc, char *argv[]) {
        if (argc != 3) {
                printf("Usage: %s <file> <length>\n", argv[0]);
                _exit(1);
        }
        const char *path = argv[1];
        u32 length = atoi(argv[2]);
        int res = truncate(path, length);
        if (res != 0) {
                perror("trunc failed");
                _exit(1);
        }
        return 0;
}
