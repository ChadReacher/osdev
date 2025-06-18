CFLAGS += -c
CFLAGS += -I ../kernel/include -I ../libk

all: boot

# Bootloader has size of `BOOT_SIZE` sectors:
# Stage1 is 1 sector
# Stage2 is 2 sectors
BOOT_SIZE = 3
BOOTLOADER = ../build/bootloader.bin

boot: $(BOOTLOADER)

$(BOOTLOADER): ../build/stage1.bin ../build/stage2.bin
	dd if=/dev/zero of=$@ bs=512 count=$(BOOT_SIZE)
	dd if=../build/stage1.bin of=$@ conv=notrunc bs=512
	dd if=../build/stage2.bin of=$@ conv=notrunc bs=512 seek=1

../build/stage2.bin: ../build/stage2.o  ../build/load_kernel.o
	$(LD) -Ttext 0x7E00 $^ -o ../build/stage2.elf
	$(OBJCOPY) -S -O binary -j .text ../build/stage2.elf ../build/stage2.bin

../build/%.bin: %.asm
	$(AS) -f bin $< -o $@

../build/%.o: %.asm
	$(AS) -f elf32 $< -o $@

../build/%.o: %.c
	$(CC) $(CFLAGS) -o $@ $<

