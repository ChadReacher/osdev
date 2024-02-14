#ifndef TIMES_H
#define TIMES_H

#include <sys/types.h>

typedef struct {
	i32 tms_utime;
	i32 tms_stime;
	i32 tms_cutime;
	i32 tms_cstime;
} tms;

i32 times(tms *buffer);

#endif
