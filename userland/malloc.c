#include <stdio.h>
#include <stdlib.h>

#define N 10000

int main(int argc, char *argv[]) {
    for (i32 i = 0; i < N; ++i) {
        void *ptr = malloc(10 * 1024);
	if (ptr == NULL) {
            printf("malloc failed\n");
	    return 1;
	}
	free(ptr);
    }
    for (i32 i = 0; i < N; ++i) {
        void *ptr = malloc(10 * 1024);
	if (ptr == NULL) {
            printf("malloc failed\n");
	    return 1;
	}
    }
    return 0;
}
