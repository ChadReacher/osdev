#include "chr_dev.h"
#include <tty.h>
#include <memory.h>
#include <vfs.h>

struct file_ops *chr_dev_ops[] = {
    // Unnamed devices
    NULL,
    // Memory devices e.g. /dev/null, /dev/zero, etc.
    &dev_memory_ops,
    // Pseudo-TTY masters e.g. /dev/ptypX
    NULL,
    // Pseudo-TTY slaves e.g. /dev/ttypX
    NULL,
    // TTY devices e.g. /dev/ttyX
    &ttyx_ops,
    // Alternate TTY devices e.g. /dev/tty, /dev/console
    &tty_ops,  // alternate tty devices
    // Parallel printer devices e.g. /dev/lpX
    NULL,
};
