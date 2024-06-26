CFLAGS += -c -I ./include -I ../libk

SUBDIRS := $(shell ls -F src | grep "\/" )
DIRS := $(SUBDIRS)
SOURCE_FILES := $(foreach d, $(DIRS), $(wildcard src/$(d)*.c) )
OBJECTS = $(patsubst %.c, %.o, $(SOURCE_FILES))
DEPENDENCIES = $(patsubst %.c, %.d, $(SOURCE_FILES))
KERNEL_DEPS = ../build/kernel_entry.o src/kernel.o \
			$(OBJECTS) ../build/interrupt.o ../build/gdt_helper.o ../build/tss_helper.o\
			../build/context_switch.o ../build/libk.a

kernel: $(DEPENDENCIES) $(KERNEL_DEPS)
	$(LD) -Tlinker.ld $(KERNEL_DEPS) -o ../build/kernel.elf
	$(OBJCOPY) -O binary ../build/kernel.elf ../build/kernel.bin

ifeq (,$(findstring $(MAKECMDGOALS),clean-all))
ifneq ($(strip $(DEPENDENCIES)), "")
  include $(DEPENDENCIES)
endif
endif

clean:
	rm -rf src/kernel.o $(OBJECTS)

clean-all:
	rm -rf src/kernel.o $(OBJECTS) $(DEPENDENCIES)

%.d: %.c
	$(CC) -I ./include/ -I ../libk -M $< -MF $*.d

%.o: %.c
	$(CC) $(CFLAGS) -o $@ $<

../build/%.o: %.asm
	$(AS) -f elf32 $< -o $@

../build/%.o: src/%.asm
	$(AS) -f elf32 $< -o $@

../build/%.o: src/core/%.asm
	$(AS) -f elf32 $< -o $@
