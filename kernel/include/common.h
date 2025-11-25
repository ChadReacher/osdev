#ifndef KERNEL_COMMON_H
#define KERNEL_COMMON_H

#define KERNEL_DS 0x10
#define KERNEL_CS 0x08

#define KIB (1024)
#define MIB ((KIB)*(KIB))

#define KERNEL_LOAD_PADDR 0x100000
#define KERNEL_LOAD_VADDR 0xC0000000

#define ALIGN_UP(val, a) (((val) + ((a) - 1)) & ~((a) - 1))
#define ALIGN_DOWN(val, a) ((val) & ~((a) - 1))
#define ALIGN(sz) ((sz / 0x10 + 1) * 0x10)

#define MIN(a,b) (((a)<(b))?(a):(b))
#define MAX(a,b) (((a)>(b))?(a):(b))

#endif
