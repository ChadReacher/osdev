#ifndef UTSNAME_H
#define UTSNAME_H

#include <sys/types.h> 

typedef struct {
	i8 sysname[9];
	i8 nodename[9];
	i8 release[9];
	i8 version[9];
	i8 machine[9];
} utsname;

i32 uname(utsname *name);

#endif
