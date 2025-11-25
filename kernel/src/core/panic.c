#include <panic.h>
#include <stdio.h>
#include <stdarg.h>
#include <serial.h>
#include <syms.h>

void debug(i8 *fmt, ...) {
    i8 buf[512] = {0};
    va_list args;

    va_start(args, fmt);
    kvsprintf(buf, fmt, args);
    va_end(args);

    write_serial(buf);
}

void panic(i8 *fmt, ...) {
    i8 buf[512] = {0};
    va_list args;

    va_start(args, fmt);
    kvsprintf(buf, fmt, args);
    va_end(args);

    write_serial("######## KERNEL PANIC ########\r\n");
    write_serial(buf);
    write_serial("Stack trace:\r\n");
    stack_trace();
    write_serial("######## System halted! ########\r\n");

    __asm__ volatile ("cli; hlt");
    while (1);
}


void stack_trace(void) {
    u32 old_ebp = 0;
    __asm__ volatile ("movl %%ebp,%0" : "=r"(old_ebp) ::);
    u32 eip = *((u32 *)old_ebp + 1);
    u32 ebp = old_ebp;
    for (u32 i = 0; i < 10; ++i) {
        if (*(u32*)ebp == 0x0) {
            break;
        }

        const char *func_name = resolve_symbol(eip);
        debug("  EIP (%#10x) at %s\r\n", eip, func_name);

        u32 new_ebp = *((u32 *)ebp);
        if (new_ebp < old_ebp) {
            debug("Stacktrace stopped: previous frame pointer inner to this frame\r\n");
            break;
        }

        old_ebp = ebp;
        ebp = new_ebp;
        eip = *((u32 *)ebp + 1);
    }
}
