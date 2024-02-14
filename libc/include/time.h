#ifndef TIME_H
#define TIME_H

#include <sys/types.h>

typedef i32 time_t;

extern time_t time(time_t *tloc);

#endif
