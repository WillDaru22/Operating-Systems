#include "rand.h"

unsigned int randseed = 1;
/* The state word must be initialized to non-zero */
int
xv6_rand(void)
{
	/* Algorithm "xor" from p. 4 of Marsaglia, "Xorshift RNGs" */
	unsigned int x = randseed;
	x ^= x << 13;
	x ^= x >> 7;
	x ^= x << 17;
	return (randseed = (x % XV6_RAND_MAX));
}

void
xv6_srand(unsigned int seed) 
{
   randseed = seed;
}
