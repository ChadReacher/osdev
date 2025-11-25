#include <types.h>
#include <elf.h>

#define SECTOR_SIZE 512

void read_sector(void *dst, u32 offset);
void disk_read(u32 pa, u32 count, u32 offset);
void disk_wait(void);

static inline u8 inb(u16 port) {
    u8 res;
    __asm__ volatile ("in %%dx, %%al" : "=a"(res) : "d"(port));
    return res;
}

static inline void insl(u16 port, void *addr, u32 cnt) {
    __asm__ volatile("cld\n\trepne\n\tinsl"
             : "=D" (addr), "=c" (cnt)
             : "d" (port), "0" (addr), "1" (cnt)
             : "memory", "cc");
}

static inline void outb(u16 port, u8 data) {
    __asm__ volatile("outb %0,%w1" : : "a" (data), "d" (port));
}

static inline void stosb(void *addr, u32 data, u32 cnt) {
    __asm__ volatile("cld; rep stosb"
            : "=D"(addr), "=c"(cnt)
            : "0"(addr), "1"(cnt), "a"(data)
            : "memory", "cc");
}

void load_kernel(void) {
    struct elf_header *elf;
    struct elf_program_header *ph;
    void (*entry)(void);

    // Load ELF header at this address
    elf = (struct elf_header *)0x10000;

    // Read the 4th sector from the disk
    disk_read((u32)elf, SECTOR_SIZE, 3);

    // Is this a valid ELF?
    if (elf->magic_number != ELF_MAGIC_NUMBER) {
        goto bad;
    }

    // Load each program segment (ignores ph flags)
    ph = (struct elf_program_header *) ((i8 *)elf + elf->phoff);
    for (int i = 0; i < elf->ph_num; ++i) {
        // paddr is the load address of this segment (as well
        // as the physical address)
        // +3 is the sector offset to the kernel in the boot image
        u32 sn = ((ph[i].offset) / SECTOR_SIZE) + 3;
        disk_read(ph[i].paddr, ph[i].memsz, sn);
        if (ph[i].memsz > ph[i].filesz) {
            u8 *pa = (u8 *)ph[i].paddr;
            stosb(pa + ph[i].filesz, 0, ph[i].memsz - ph[i].filesz);
        }
    }

    // call the entry point from the ELF header
    // note: does not return!
    entry = (void(*)(void))(elf->entry);
    entry();

bad:
    while (1)
        /* do nothing */;
}

// Read 'count' bytes staring from 'sn' sector number into physical address 'pa'.
// Might copy more than asked
void disk_read(u32 pa, u32 count, u32 sn) {
    u32 end_pa;

    end_pa = pa + count;

    // round down to sector boundary
    pa &= ~(SECTOR_SIZE - 1);

    while (pa < end_pa) {
        read_sector((u8*) pa, sn);
        pa += SECTOR_SIZE;
        ++sn;
    }
}

void read_sector(void *dst, u32 offset) {
    // wait for disk to be ready
    disk_wait();

    outb(0x1F2, 1);     // count = 1
    outb(0x1F3, offset);
    outb(0x1F4, offset >> 8);
    outb(0x1F5, offset >> 16);
    outb(0x1F6, (offset >> 24) | 0xE0);
    outb(0x1F7, 0x20);  // cmd 0x20 - read sectors

    // wait for disk to be ready
    disk_wait();

    // read a sector
    insl(0x1F0, dst, SECTOR_SIZE/4);
}

void disk_wait(void) {
    while ((inb(0x1F7) & 0xC0) != 0x40) {
        /* do nothing */;
    }
}

