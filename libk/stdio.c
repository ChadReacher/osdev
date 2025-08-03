#include "stdio.h"
#include "stdlib.h"
#include "screen.h"
#include "string.h"
#include "panic.h"
#include "ctype.h"

void kprintf(i8 *fmt, ...) {
    i8 internal_buf[2048];
    va_list args;

    if (!fmt) {
        return;
    }
    va_start(args, fmt);
    memset(internal_buf, 0, sizeof internal_buf);
    kvsprintf(internal_buf, fmt, args);
    debug("Unimplemented: ");
    debug(internal_buf);
    /*screen_print_string(internal_buf);*/
    va_end(args);
}

static void hex(u32 num, char **dest_buf, char *internal_buf_p, u8 *alt_conversion, u32 *leading_zeroes) {
    char temp[20] = {0};
    u32 idx = 0;
    for (u32 i = 0; i < *leading_zeroes; ++i) {
        internal_buf_p[i] = '0';
    }
    if (*alt_conversion == 1) {
        internal_buf_p[0] = '0';
        internal_buf_p[1] = 'x';
        idx = 2;
    }
    utoa(num, temp, 16);
    if (*leading_zeroes) {
      idx = *leading_zeroes - strlen(temp);
    }
    memcpy(internal_buf_p + idx, temp, strlen(temp));
    u32 sz = strlen(internal_buf_p);
    memcpy(*dest_buf, internal_buf_p, sz);
    *dest_buf += sz;
}

void kvsprintf(i8 *buf, i8 *fmt, va_list args) {
    i8 internal_buf[512] = {0};
    i8 leading_number[3] = {0};
    u32 sz;
    i8 *p;
    i8 *temp_s;
    i8 c;
    u8 percent = 0;
    bool alt_conversion = true;
    u32 leading_zeroes = 0;

    for (p = fmt; *p; ++p) {
        if (*p != '%' && percent == 0) {
            *buf = *p;
            ++buf;
            continue;
        }
        if (*p == '%') {
            ++p;
        }
        switch (*p) {
            case 'd':
            case 'i':
                memset(internal_buf, 0, sizeof internal_buf);
                itoa(va_arg(args, i32), internal_buf, 10);
                sz = strlen(internal_buf);
                memcpy(buf, internal_buf, sz);
                buf += sz;
                percent = 0;
                break;
            case 'x':
                memset(internal_buf, 0, sizeof internal_buf);
                hex(va_arg(args, u32), &buf, internal_buf, &alt_conversion, &leading_zeroes);
                percent = 0;
                leading_zeroes = 0;
                alt_conversion = 0;
                break;
            case 'c':
                c = va_arg(args, i32);
                if (c) {
                    *buf = c;
                    ++buf;
                }
                percent = 0;
                break;
            case 's':
                temp_s = va_arg(args, i8*);
                sz = strlen(temp_s);
                memcpy(buf, temp_s, sz);
                buf += sz;
                percent = 0;
                break;
            case 'p':
                memset(internal_buf, 0, sizeof internal_buf);
                utoa((u32)va_arg(args, void*), internal_buf, 16);
                sz = strlen(internal_buf);
                memcpy(buf, internal_buf, sz);
                buf += sz;
                percent = 0;
                break;
            case '%':
                *buf = *p;
                ++buf;
                break;
            case '#':
                percent = 1;
                alt_conversion = 1;
                break;
            case '0':
                percent = 1;
                memset(leading_number, 0, sizeof leading_number);
                u32 i = 0;
                ++p;
                while (isdigit(*p)) {
                    if (i < 3) {
                        leading_number[i] = *p;
                        ++i;
                    }
                    ++p;
                }
                leading_zeroes = atoi(leading_number);
                --p;
                break;
            default:
                break;
        }
    }
}
