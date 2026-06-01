#include <stdio.h>
#include <sys/mman.h>
#include <errno.h>

int main()
{
	void *addr, *addr1;
	unsigned long *intaddr;
	size_t region_length;
	int protection;
	int flags;
	unsigned long tmp = 0, i, j, pg = 1024*4;

	addr = (void *)0x5000000000;
	region_length = sizeof(char) * 1024 * 1024  * 1024 * 32;
	//region_length = sizeof(char) * 1024 * 1024  * 32;
	protection = PROT_READ | PROT_WRITE;
	flags = MAP_SHARED | MAP_FIXED | MAP_NORESERVE | MAP_ANONYMOUS;
	
	//addr = mmap(addr, region_length, protection, flags, -1, 0); 
	//addr1 = mmap(addr, region_length, protection, flags, -1, 0);

	for(i = 0; i < 32750; i++)
	{
		intaddr = (unsigned long *)(addr+(i*pg));
		*intaddr = i;
	}

	for(i = 0;i < 32750;i++)
	{
		intaddr = (unsigned long *)(addr+(i*pg));
		tmp += (*intaddr);
	}

	printf("sum %ld", tmp);
	
	printf("address : %llx\n", addr);
	printf("Erro : %d", errno);
	getchar();
	return 0;
}
