#include <sys/types.h>
#include <unistd.h>
#include "cheader.h"
int main(int argc, char** argv)
{
	void *caster, *rights; 
	extent etrial;
	user_file_rights rtrial;
	int rw;
	char sizeunit;
	int size;
	unsigned long size_b = 0, usize;

	printf("<app> <r|w|rw> size<MB|GB|Pages> (page size = 4K)\n");

	if(argc < 3)
		return 0;

	if(!strcmp(argv[1], "rw"))
		rw = 0x3;
	else if(!strcmp(argv[1], "r"))
		rw = 0x2;
	else if(!strcmp(argv[1], "w"))
		rw = 0x1;
	else
		rw = 0x0;

	sscanf(argv[2], "%d%c", &size, &sizeunit);
	printf("size %d unit %c\n", size, sizeunit|32);
	usize = (unsigned long)size;
	if((sizeunit|32) == 'g')
		size_b = usize * 1024 * 1024 * 1024;
	else if((sizeunit|32) == 'm')
		size_b = usize * 1024 * 1024;
	else if((sizeunit|32) == 'p')
		size_b = usize * 4 * 1024;

	printf("rw bit used %d\n", rw);

	etrial.base = 0x8000000000;
	etrial.size = size_b;

	printf("getuid=%d\n", getuid());
	rtrial.uid = getuid();
	rtrial.rw = rw;

	printf("size in bytes : %lx\n", size_b);

	caster = (void *)&etrial;
	rights = (void *)&rtrial;

	start_timer();
	printf("error : %d\n", syscall(313, caster, rights, 1));
	end_timer();

	print_time_diff();
	return 0;
}
