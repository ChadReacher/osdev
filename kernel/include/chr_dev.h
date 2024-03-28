#ifndef CHR_DEV
#define CHR_DEV

#include <types.h>

#define READ 0
#define WRITE 1

#define MAJOR(a) ((unsigned)(a)>>8)
#define MINOR(a) ((a)&0xFF)

i32 char_write(u16 dev, i8 *buf, u32 count);
i32 char_read(u16 dev, i8 *buf, u32 count);

#endif
