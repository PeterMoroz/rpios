
int memcmp(const void *p1, const void *p2, int nbytes)
{
	const unsigned char *s1 = p1, *s2 = p2;
	for (int i = 0; i < nbytes; i++) {
		int v = s1[i] - s2[i];
		if (v)
			return v;
	}
	return 0;
}

void* memcpy(void *dst, const void *src, int nbytes) {
	unsigned char *d = dst;
	const unsigned char *s = src;
	for (int i = 0; i < nbytes; i++)
		d[i] = s[i];
	return dst;
}

void* memset(void *p, char c, int n) {
	char *b = p, *e = p + n;
	while (b < e)
		*b++ = c;
	return p;
}

