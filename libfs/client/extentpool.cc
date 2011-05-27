#include "extentpool.h"
#include <assert.h>
#include <fcntl.h>
#include <stdint.h>
#include <sys/mman.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <unistd.h>
#include "common/util.h"
#include "common/hrtime.h"


ExtentPool::ExtentPool(rpcc *c, unsigned int principal_id): 
	client_(c), 
	principal_id_(principal_id)
{
	fd_ = open("extentpool.vistaheap", O_RDWR);
	assert(fd_ > -1);
}


unsigned long long 
ExtentPool::CreateExtent(size_t size)
{
	unsigned long long extent_id;
	unsigned long long usize = (unsigned long long) size;

	int intret = client_->call(22, principal_id_, usize, extent_id);
	assert(intret == 0);

	return (unsigned long long ) extent_id;
}


int 
ExtentPool::DeleteExtent(unsigned long long extent_id, size_t size)
{
	int                rv;
	unsigned long long usize = (unsigned long long) size;

	int intret = client_->call(23, principal_id_, extent_id, usize, rv);
	assert(intret == 0);

	return rv;
}


void *
ExtentPool::AccessExtent(unsigned long long extent_id, size_t size)
{
	int    rv;
	size_t round_size = num_pages(size) * kPageSize;
	
	// TODO: Move functionality inside the kernel.
	// The kernel must implement the access_extent system call, which
	// ensures that a principal has the proper access rights. For now,
	// we check with the privileged server that we can access the 
	// extent and then mmap it in our address space. Of course this
	// is not bullet-proof but implements the logic and serves our 
	// prototype well for now.

	int intret = client_->call(24, principal_id_, extent_id, rv);
	assert(intret == 0);
	
	if (rv == 0) {
		return mmap(0, size, PROT_WRITE|PROT_READ, MAP_SHARED, fd_, 0);
	} else {
		return NULL;
	}
}
