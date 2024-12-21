#include "chr_dev.h"
#include "tty.h" 
#include "errno.h"
#include "process.h"
#include <vfs.h>

struct file_ops *chr_dev_ops[] = {
	NULL,     /* no dev */
	NULL,     /* dev mem */
	NULL,     /* dev fd */
	NULL,	  /* dev hd */
	&ttyx_ops,  /* dev ttyx */
	&tty_ops,	  /* dev tty */
	NULL,     /* dev lp */
};
