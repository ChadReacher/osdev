void dummy_test_entrypoint() {
}

__attribute__ ((section ("kernel_entry"))) void _start() {
	char *video_memory = (char*) 0xB8000;
	char *str = "Hello, world from kernel!";

	while (*str != 0) {
		*video_memory = *str;
		*(video_memory + 1) = 0x0F;
		video_memory += 2;
		++str;
	}
}
