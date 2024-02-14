CFLAGS += -c -I ../libc

binaries = bin/init bin/test bin/sh bin/ls bin/cat bin/rm

all: userland

.PHONY: userland
userland: ../build/crt0.o $(binaries)

../build/crt0.o: crt0.S
	nasm -f elf32 $< -o $@

bin/%: %.c
	$(CC) $(CFLAGS) $< -o ../build/$*.o
	$(LD) -Ttext 0x0 -o $@ ../build/crt0.o ../build/$*.o ../build/libc.a
