#ifndef __STAMNOS_TESTFW_UTIL_H
#define __STAMNOS_TESTFW_UTIL_H

static void fillbuf(char *buf, int n, unsigned int seed)
{
	int i;

	srand(seed);
	for (int i=0; i<n; i++) {
		buf[i] = rand() % 256;
	}
}	

#endif //__STAMNOS_TESTFW_UTIL_H
