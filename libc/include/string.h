#ifndef STRING_H
#define STRING_H

#include "sys/types.h"

void *memset(void *ptr, u32 value, u32 num);
void *memcpy(void *dst, const void* src, u32 num); 

u32 strlen(const i8 *str);
i8 *strrev(i8 *str);
u8 strncmp(const i8 *left, const i8 *right, i32 len);
i8 *strcpy(i8 *dest, const i8 *src);
i8 *strncpy(i8 *dest, const i8 *src, u32 len);
u32 strcspn(const i8 *str1, const i8 *str2);
i8 *strsep(i8 ** str, const i8 *sep);
i32 strcmp(const i8 *str1, const i8 *str2);
i8 *strdup(const i8 *str);
i8 *strcat(i8 *dest, const i8 *src);
i8 *strchr(const i8 *s, i32 c);

i8 *strerror(i32 errnum);

#endif
