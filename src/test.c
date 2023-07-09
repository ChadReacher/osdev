#include "test.h"

void test() {
	__asm__ __volatile__ ("int $80" : : "a"(0));
}
