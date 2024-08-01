#ifndef __STRUTILS_H__
#define __STRUTILS_H__

#include <stddef.h>

char* strcat(char *dst, const char *src);
int strcmp(const char *s1, const char *s2);
char* strcpy(char *s1, const char *s2);
size_t strlen(const char *s);
int strncmp(const char *s1, const char *s2, size_t n);

#endif
