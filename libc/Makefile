CFLAGS += -c -I ./

LIBC_OBJ = $(shell find . -type f -name "*.c")
LIBC_OBJ := $(LIBC_OBJ:./%.c=../build/libc/%.o)

all: $(LIBC_OBJ)
	$(AR) -rcs ../build/libc.a $^

../build/libc/%.o: %.c
	$(CC) $(CFLAGS) $< -o $@
