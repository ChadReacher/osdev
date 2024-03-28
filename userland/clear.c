#include <string.h>
#include <unistd.h> 

void main() {
	const char *escape_clear = "\033[;H\033[J";
	write(STDOUT_FILENO, escape_clear, strlen(escape_clear));
}
