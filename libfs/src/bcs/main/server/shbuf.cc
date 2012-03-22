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
#include "common/mmapregion.h"


namespace server {


int
SharedBuffer::Init(const char* suffix)
{
	int         ret;
	int         fd;
	const char* root = "/tmp/shbuf_";
	size_t      total_size;
	size_t      header_size;
	void*       mmap_base;

	path_ = std::string(root) + std::string(suffix);
	size_ = SharedBufferManager::RuntimeConfig::sharedbuffer_size;

	DBG_LOG(DBG_DEBUG, DBG_MODULE(server_bcs), "SharedBuffer: path = %s, size = %" PRIu64 "\n", 
	        path_.c_str(), size_);

	return Open(path_.c_str(), size_, kCreate | kMap, this);
}


} // namespace server
