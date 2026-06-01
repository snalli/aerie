#include <stdio.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <stdlib.h>
#include <errno.h>

#define PAGE_SIZE (1024*4)
#define PRESENT ((unsigned long long)1<<63)
#define SWAP ((unsigned long long)1<<62)
#define SHIFT ((unsigned long long)0x111111<<55)

int main(int argc, char **argv)
{
	pid_t pid;
	unsigned long addr;
	int fd;
	char filename[100];	
	unsigned long long virt_pgno;
	unsigned long long virt_addr;
	unsigned long long phys;
	unsigned long long tmp;
 
	if(argc < 3)
	{
		printf("<app> <pid> <addr>\n");
		return 0;
	}

	pid = atoi(argv[1]);
	virt_addr = strtoull(argv[2], NULL, 16);

	sprintf(filename, "/proc/%d/pagemap\0", pid);
	printf("opening file %s\n", filename);
	fd = open(filename, O_RDONLY);

	virt_pgno = virt_addr / PAGE_SIZE  * sizeof(unsigned long long);
	lseek64(fd, virt_pgno, SEEK_SET);
	read(fd, &phys, sizeof(unsigned long long));

	printf("virt addr : \t%lx\n", virt_addr);
	printf("index : \t%lx\n", virt_pgno);
	printf("phys page : \t%lx\n", phys); 

	printf("***********\n");
	printf("Present : %s\n", phys & PRESENT ? "yes":"no");
	printf("Swapped : %s\n", phys & SWAP ? "yes":"no");
	printf("Page Size : %x\n", (phys & SHIFT)>>55);
	
	close(fd);
	return 0;
}
