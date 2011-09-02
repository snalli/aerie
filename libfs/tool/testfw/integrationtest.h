#ifndef _TESTFW_INTEGRATIONTEST_H_AFS156
#define _TESTFW_INTEGRATIONTEST_H_AFS156

#include "tool/testfw/unittest.h"
#include <fcntl.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <sys/mman.h>
#include <unistd.h>
#include <assert.h>


static void* 
OpenAndMap(const char* pathname, int flags, int len)
{
	int fd;

	if (!pathname) {
		return NULL;
	}

	if (flags & O_CREAT) {
		fd = open(pathname, O_CREAT|O_TRUNC|O_RDWR, S_IRUSR|S_IRGRP|S_IROTH|S_IWUSR|S_IWGRP|S_IWOTH);
		assert(fd>0);
		assert(ftruncate(fd, len)==0);
	} else {
		fd = open(pathname, O_RDWR);
		assert(fd>0);
	}

	return mmap(0, len, PROT_READ|PROT_WRITE, MAP_SHARED, fd, 0);
}




//
// Definition of some useful fixtures follows
//


// 
// Shared Memory Region fixture
//
// This fixture enables the intra-process sharing between tests running in
// separate processes
//

#define DEFINE_SHARED_MEMORY_REGION_FIXTURE(fixture_name, region_type)       \
static const char* __pathname = "/tmp/"#region_type;                         \
struct fixture_name                                                          \
{                                                                            \
	fixture_name()                                                           \
	{                                                                        \
		region_ = (region_type*) OpenAndMap(__pathname, 0,                   \
		sizeof(region_type));                                                \
	}                                                                        \
                                                                             \
	~fixture_name()                                                          \
	{                                                                        \
		munmap(region_, sizeof(fixture_name));                               \
	}                                                                        \
                                                                             \
	region_type* region_;                                                    \
                                                                             \
	static int InitRegion(void*);                                            \
};



#endif /* _TESTFW_INTEGRATIONTEST_H_AFS156 */
