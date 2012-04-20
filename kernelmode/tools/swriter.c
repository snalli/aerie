#include <stdio.h>
#include <sys/mman.h>
#include <errno.h>

int main()
{
	void *addr;
	unsigned long *intaddr;
	unsigned long tmp = 0, i, pg = 1024*4;

	addr = (void *)0x8000000000;

	for(i = 0; i < 10; i++)
	{
		intaddr = (unsigned long *)(addr+(i*pg));
		*intaddr = (i*i);
	}

	return 0;
}
