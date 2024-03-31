#include <unistd.h>
#include <stdio.h>
#include <stdlib.h>

int main(int argc, char **argv) {
	int n;

	if (argc != 2) {
		printf("\tusage: sleep time\n");
		_exit(1);
	}
	n = atoi(argv[1]);
	sleep(n);
	return 0;
}
