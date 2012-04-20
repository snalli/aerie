#include <stdio.h>
#include <sys/mman.h>
#include <errno.h>

int main()
{
	void *addr;
	size_t region_length;
	int protection;
	int flags;

	getchar();
	addr = (void *)0x5000000000;
	region_length = sizeof(char) * 1024 * 1024  * 1024 * 32;
	protection = PROT_NONE;
	flags = MAP_SHARED | MAP_ANONYMOUS | MAP_FIXED | MAP_NORESERVE;
	
	addr = mmap(addr, region_length, protection, flags, -1, 0); 


	printf("address : %llx\n", addr);
	printf("Erro : %d", errno);
	getchar();
	return 0;
}
