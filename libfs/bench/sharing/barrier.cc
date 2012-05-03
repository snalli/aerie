#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <sys/mman.h>
#include <assert.h>
#include <unistd.h>
#include "common/ut_barrier.h"


void BarrierInit(int count)
{
	int fd;
	ut_barrier_t *bp;
	fd = open("/tmp/sharing_barrier", O_CREAT|O_RDWR, S_IRUSR|S_IWUSR);
	assert(fd>0);
	ftruncate(fd, 1024);
	bp = (ut_barrier_t*) mmap(0, 1024, PROT_READ|PROT_WRITE, MAP_SHARED, fd, 0);
	ut_barrier_init(bp, count, true);
}


void BarrierWait()
{
	int fd;
	ut_barrier_t *bp;
	fd = open("/tmp/sharing_barrier", O_CREAT|O_RDWR, S_IRUSR|S_IWUSR);
	assert(fd>0);
	ftruncate(fd, 1024);
	bp = (ut_barrier_t*) mmap(0, 1024, PROT_READ|PROT_WRITE, MAP_SHARED, fd, 0);
	ut_barrier_wait(bp);
}
