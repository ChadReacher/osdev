#include <unistd.h>
#include <stdio.h>

i32 main(i32 argc, i8 *argv[]) {
    if (argc < 2) {
        printf("\t usage: %s file\n", argv[0]);
        return 1;
    }

    for (i32 i = 1; i < argc; ++i) {
        if (unlink(argv[i]) != 0) {
            perror("rm failed");
        }
    }

    return 0;
}
