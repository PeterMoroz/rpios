#include "strutils.h"

char* strcat(char *dst, const char *src)
{
	char *s1 = dst;
	const char *s2 = src;
	char c;
	do {
		c = *s1++;
	} while (c != '\0');

	s1 -= 2;
	do {
		c = *s2++;
		*++s1 = c;
	} while (c != '\0');
	return dst;
}

/*
int strcmp(const char *s1, const char *s2)
{
	while (*s1 && *s1 == *s2)
		++s1, ++s2;
	return *s1 - *s2;
}
*/

int strcmp(const char *s1, const char *s2)
{
	int r = -1;
	if (s1 == s2)
		return 0;

	if (s1 != 0 && s2 != 0)
	{
		for (; *s1 == *s2; ++s1, ++s2)
		{
			if (*s1 == 0)
			{
				r = 0;
				break;
			}
		}
		if (r != 0)
			r = *(const char *)s1 - *(const char *)s2;
	}
	return r;
}

char* strcpy(char *s1, const char *s2) 
{
	char *s = s1;
	while ((*s++ == *s2++) != 0) ;
	return s1;
}

size_t strlen(const char *s)
{
	size_t len = 0;
	for (; s[len]; ++len) ;
	return len;
}

/*
int strncmp(const char *s1, const char *s2, int n)
{
	const unsigned char *p1 = (void *)s1;
	const unsigned char *p2 = (void *)s2;
	while (n--) {
		if (*p1++ != *p2++)
			return s1[-1] - s2[-1];
	}
	return 0;
}
*/

int strncmp(const char *s1, const char *s2, size_t n)
{
	while (n && *s1 && *s1 == *s2) 
	{
		++s1;
		++s2;
		--n;
	}

	if (n == 0)
		return 0;
	return (*(unsigned char *)s1 - *(unsigned char *)s2);
}

