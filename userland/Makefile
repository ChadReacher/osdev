CFLAGS += -c -I ../libc/include

binaries = bin/init bin/test bin/sh bin/ls bin/cat bin/rm bin/link bin/mkdir\
		   bin/rmdir bin/mv bin/clear bin/sleep bin/wc bin/symlink bin/readlink\
		   bin/trunc bin/mount bin/umount

all: userland

.PHONY: userland
userland: ../build/crt0.o $(binaries)

../build/crt0.o: crt0.S
	nasm -f elf32 $< -o $@

bin/%: %.c
	$(CC) $(CFLAGS) $< -o ../build/$*.o
	$(LD) -Ttext 0x0 -o $@ ../build/crt0.o ../build/$*.o ../build/libc.a
