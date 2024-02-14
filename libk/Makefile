CFLAGS += -c -I ../kernel/include/ -I ./

LIBK_OBJ = $(shell find . -type f -name "*.c")
LIBK_OBJ := $(LIBK_OBJ:./%.c=../build/libk/%.o)

all: $(LIBK_OBJ)
	$(AR) -rcs ../build/libk.a $^

../build/libk/%.o: %.c
	$(CC) $(CFLAGS) $< -o $@
