#ifndef BLK_DEV
#define BLK_DEV

#include <types.h>

#define READ 0
#define WRITE 1

#define NRBLKDEV 7

#define BLOCK_SIZE 1024

#define MAJOR(a) ((unsigned)(a)>>8)
#define MINOR(a) ((a)&0xFF)

void rw_block(u32 rw, u16 dev, u32 block, i8 **buf);
i32 block_write(u16 dev, u32 *pos, i8 *buf, u32 count);
i32 block_read(u16 dev, u32 *pos, i8 *buf, u32 count);

#endif
