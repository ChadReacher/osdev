#ifndef SYMBOLS_H
#define SYMBOLS_H

#include <types.h>

struct ksym {
    u32 addr;
    char *name;
};

extern u32 ksyms_count;
extern struct ksym ksymbols[];

static const char *resolve_symbol(u32 eip) {
    const char *best_match = "??";
    u32 best_addr = 0x0;

    for (u32 i = 0; i < ksyms_count; ++i) {
        if (eip >= ksymbols[i].addr && ksymbols[i].addr >= best_addr) {
            best_match = ksymbols[i].name;
            best_addr = ksymbols[i].addr;
        }
    }
    return best_match;
}

#endif
