#include "bcs/main/client/shbuf.h"
#include <inttypes.h>
#include <stdlib.h>
#include <sys/types.h>
#include <sys/mman.h>
#include <sys/file.h>
#include <fcntl.h>
#include <unistd.h>
#include "bcs/main/common/debug.h"
#include "common/util.h"
#include "common/errno.h"

namespace client {


int
SharedBuffer::Map()
{
	int         ret;
	int         fd;

	if ((fd = open(path_.c_str(), O_RDWR)) < 0) {
		return -E_ERRNO;
	}
	if ((base_ = mmap(0, size_, PROT_WRITE | PROT_READ, MAP_SHARED, fd, 0)) 
	    == (void*) -1) 
	{
		return -E_ERRNO;
	}
	
	DBG_LOG(DBG_DEBUG, DBG_MODULE(server_bcs), 
	        "SharedBuffer: path = %s, size = %" PRIu64 ", base = %p\n", 
	        path_.c_str(), size_, base_);

	return E_SUCCESS;
}




} // namespace client
