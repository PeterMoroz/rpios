#include "log2.h"

/* The algorithm to compute log2 of 32-integer is borrowed here:
 * http://graphics.stanford.edu/~seander/bithacks.html#IntegerLogLookup
 */

#define LT(n) n, n, n, n, n, n, n, n, n, n, n, n, n, n, n, n
static const char LogTable256[256] =
{
    -1, 0, 1, 1, 2, 2, 2, 2, 3, 3, 3, 3, 3, 3, 3, 3,
    LT(4), LT(5), LT(5), LT(6), LT(6), LT(6), LT(6),
    LT(7), LT(7), LT(7), LT(7), LT(7), LT(7), LT(7), LT(7)
};

uint32_t log2(uint32_t v)
{
	uint32_t r, t, tt;
	if (tt = v >> 16)
	{
	    r = (t = tt >> 8) ? 24 + LogTable256[t] : 16 + LogTable256[tt];
	}
	else 
	{
	    r = (t = v >> 8) ? 8 + LogTable256[t] : LogTable256[v];
	}
	return r;
}

