#include <stdio.h>
#include <unistd.h>
#include <fcntl.h>
#include <string.h>
#include <stdlib.h>
#include <fcntl.h>

#define BUFSZ 1024

i8 buf[BUFSZ];

i32 main(i32 argc, i8 *argv[]) {
    i32 i, j, fd, r, err;

    if (argc == 1) {
        while ((r = read(0, buf, BUFSZ)) > 0) {
            if ((err = write(1, buf, r)) < 0) {
                write(stderr, "error :(", 8);
                _exit(1);
                //perror("cat: write failed");
            }
            printf("%d\n", err);
        }
        return 0;
    }

    for (i = 1; i < argc; ++i) {
        memset(buf, 0, BUFSZ);
        fd = open(argv[i], O_RDONLY, 0);
        if (fd < 0) {
            perror("cat: couldn't open file");
            continue;
        }
        do {
            r = read(fd, buf, BUFSZ);
            if (r < 0) {
                write(stderr, "error on read :(", 16);
                break;
            } else if (r == 0) {
                break;
            }
            for (j = 0; j < r; ++j) {
                err = write(stdout, buf + j, 1);
                if (err < 0) {
                    write(stderr, "error on write :(", 17);
                    break;
                }
            }
        } while (err >= 0);

        close(fd);
    }

    return 0;
}
