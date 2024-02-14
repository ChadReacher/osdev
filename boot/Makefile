BOOT_ASM = ../build/stage1.bin ../build/stage2.bin ../build/font.bin
BOOTLOADER = ../build/bootloader.bin

AS = nasm

all: boot

boot: $(BOOT_ASM)
	cat $^ > $(BOOTLOADER)

../build/%.bin: %.asm
	$(AS) -f bin $< -o $@
