#include <stdio.h>
#include <syscall.h>

int main(int argc, char** argv)
{
	unsigned long v_addr, size_mb;

	if(argc <2)
	{
		printf("<app> <size>");
		return 0;
	}

	v_addr = 0x8000000000;
	size_mb = (unsigned long)atoi(argv[1]);

	printf("error : %d", syscall(312, v_addr, size_mb));
	return 0;
}

