#ifndef _SYS_TYPES_H
#define _SYS_TYPES_H

#define NULL ((void *) 0)

typedef unsigned char		u8;
typedef unsigned short		u16;
typedef unsigned int		u32;
typedef unsigned long long	u64;
typedef char				i8;
typedef short				i16;
typedef int					i32;
typedef long long			i64;
typedef float				f32;
typedef double				f64;
typedef unsigned char		bool;

#define true  (1)
#define false (0)

typedef u16 dev_t;
typedef u8 gid_t;
typedef u16 ino_t;
typedef u16 mode_t;
typedef u16 nlink_t;
typedef u16 off_t;
typedef i32 pid_t;
typedef u16 uid_t;

#endif
