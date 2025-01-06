CFLAGS += -c
CFLAGS += -I ../kernel/include -I ../libk

all: boot

# Bootloader is `BOOT_SIZE_SECT` sectors sized:
# Stage1 is 1 sector
# Stage2 is 2 sectors
BOOT_SIZE_SECT = 3

boot: ../build/stage1.bin ../build/stage2.bin
	dd if=/dev/zero of=../build/bootloader.bin bs=512 count=$(BOOT_SIZE_SECT)
	dd if=../build/stage1.bin of=../build/bootloader.bin conv=notrunc bs=512
	dd if=../build/stage2.bin of=../build/bootloader.bin conv=notrunc bs=512 seek=1

../build/stage2.bin: ../build/stage2.o  ../build/load_kernel.o
	$(LD) -Ttext 0x7E00 $^ -o ../build/stage2.elf
	$(OBJCOPY) -S -O binary -j .text ../build/stage2.elf ../build/stage2.bin

../build/stage1.bin: stage1.asm
	$(AS) -f bin $< -o $@

../build/%.o: %.asm
	$(AS) -f elf32 $< -o $@

../build/%.o: %.c
	$(CC) $(CFLAGS) -o $@ $<

