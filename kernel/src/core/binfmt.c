#include "binfmt.h"
#include "errno.h"

struct binfmt {
	i32 (*load_binary)(struct binparams *, struct registers_state *regs);
};


i32 elf_load_binary(struct binparams *bprm);

struct binfmt binary_formats[] = {
	{ elf_load_binary },
};

i32 search_binary_handler(struct binparams *bprm, struct registers_state *regs) {
	i32 retval = -1;
	for (u32 i = 0; i < sizeof(binary_formats) / sizeof(binary_formats[0]); ++i) {
		struct binfmt *binfmt = binary_formats + i;
		retval = binfmt->load_binary(bprm, regs);
		if (retval == 0) {
			break;
		} else if (retval == -ENOEXEC) {
			continue;
		} else {
			return retval;
		}
	}
	return retval;
}
