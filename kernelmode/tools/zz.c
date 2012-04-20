#include <stdio.h>

#define PERS_SPACE 1024*1024*1024*1
#define PAGESIZE   4*1024
#define EXTENTSIZE PAGESIZE 
//#define PAGEPROTMAPSIZE (unsigned long)PERS_SPACE/(unsigned long)EXTENTSIZE
#define PAGEPROTMAPSIZE (unsigned long)PERS_SPACE/(unsigned long)EXTENTSIZE

int main()
{
	unsigned long tmp;
	tmp = PERS_SPACE;
	tmp /= EXTENTSIZE;
	tmp *= 2;
	tmp /= sizeof(unsigned long);
	printf("extent:%lx pageprot:%lx", EXTENTSIZE, tmp);
	return 0;
}
