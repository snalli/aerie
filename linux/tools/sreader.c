#include <stdio.h>
#include <sys/mman.h>
#include <errno.h>

int main()
{
	void *addr;
	unsigned long *intaddr;
	unsigned long tmp = 0, i, j, pg = 1024*4;
	unsigned long pages = 0;


	pages = 4UL*1024UL*1024UL;
	addr = (void *)0x8000000000;

	start_timer();
	for(i = 0;i < pages;i++)
	{
		intaddr = (unsigned long *)(addr+(i*pg));
		tmp += (*intaddr);
	}
	end_timer();
	print_time_diff();

	printf("sum %ld", tmp);

	return 0;
}
