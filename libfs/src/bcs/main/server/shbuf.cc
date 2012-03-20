#include "bcs/main/server/shbuf.h"
#include <inttypes.h>
#include <stdlib.h>
#include <sys/types.h>
#include <sys/mman.h>
#include <sys/file.h>
#include <fcntl.h>
#include <unistd.h>
#include "bcs/main/common/debug.h"
#include "bcs/main/server/shbufmgr.h"
#include "spa/spa.h"
#include "common/util.h"

namespace server {


int
SharedBuffer::Init(const char* suffix)
{
	int         ret;
	int         fd;
	const char* root = "/tmp/shbuf_";

	size_ = RoundUpSize(SharedBufferManager::RuntimeConfig::sharedbuffer_size, kBlockSize);
	path_ = std::string(root) + std::string(suffix);
	
	DBG_LOG(DBG_DEBUG, DBG_MODULE(server_bcs), "SharedBuffer: path = %s, size = %" PRIu64 "\n", 
	        path_.c_str(), size_);

	// file permissions must be set to allow just the server/client pair
	// access to the shared buffer
	if ((fd = open(path_.c_str(), O_RDWR | O_CREAT | O_TRUNC, 0666)) < 0) {
		return -E_ERRNO;
	}
	if (ftruncate(fd, size_) < 0) {
		return -E_ERRNO;
	}
	if ((base_ = mmap(0, size_, PROT_WRITE | PROT_READ, MAP_SHARED, fd, 0)) 
	    == (void*) -1) 
	{
		return -E_ERRNO;
	}

	return E_SUCCESS;
}


int SharedBuffer::Consume()
{

}

} // namespace server
