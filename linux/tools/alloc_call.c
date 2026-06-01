#include <stdio.h>
#include <syscall.h>

int main(int argc, char** argv)
{
	unsigned long v_addr, offset, size_mb, base;
	base = 0x8000000000;

	if(argc < 3)
	{
		printf("Base address is 0x8000000000\n");
		printf("<app> <offset from base in MB> <size in MB>\n");
		return 0;
	}

	offset = (unsigned long)atoi(argv[1]);
	size_mb = (unsigned long)atoi(argv[2]);
	v_addr = base + (offset * 1024 * 1024);

	printf("Memory block request\n");
	printf("--------------------\n");
	printf("VA to be mapped : %lx\n", v_addr);
	printf("Area to map : %ldMB\n", size_mb); 

	printf("ERROR : %d", syscall(312, v_addr, size_mb));
	return 0;
}

