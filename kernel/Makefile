CFLAGS += -c
CFLAGS += -I ../kernel/include -I ../libk

SOURCE_FILES := $(shell find src -name '*.c')
OBJECTS = $(patsubst %.c, %.o, $(SOURCE_FILES))
DEPENDENCIES = $(patsubst %.c, %.d, $(SOURCE_FILES))
ASM_FILES := $(shell find ./ -name '*.asm')
OBJECTS += $(patsubst %.asm, %.o, $(ASM_FILES))
KERNEL_DEPS = $(OBJECTS) ../build/libk.a

kernel: $(DEPENDENCIES) $(KERNEL_DEPS)
	$(LD) $(KERNEL_DEPS) -o ../build/kernel.elf -T linker.ld $(LDFLAGS)
	cp ../build/kernel.elf ../build/kernel.bin
	#$(OBJCOPY) -S ../build/kernel.elf ../build/kernel.bin

ifeq (,$(findstring $(MAKECMDGOALS),clean-deps))
ifneq ($(strip $(DEPENDENCIES)), "")
  include $(DEPENDENCIES)
endif
endif

clean:
	rm -rf $(OBJECTS)

clean-deps:
	rm -rf $(OBJECTS) $(DEPENDENCIES)

%.d: %.c
	$(CC) -I ./include/ -I ../libk -M $< -MF $*.d

%.o: %.c
	$(CC) $(CFLAGS) -o $@ $<

%.o: %.asm
	$(AS) -f elf32 $< -o $@
