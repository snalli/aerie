#include "bcs/main/client/shbuf.h"
#include <inttypes.h>
#include <stdlib.h>
#include <sys/types.h>
#include <sys/mman.h>
#include <sys/file.h>
#include <fcntl.h>
#include <unistd.h>
#include "bcs/main/client/ipc.h"
#include "bcs/main/common/debug.h"
#include "common/util.h"
#include "common/errno.h"

namespace client {


int
SharedBuffer::Map()
{
	int ret;
	int fd;
	
	if ((ret = Open(path_.c_str(), 0, kMap, this)) < 0) {
		return ret;
	}

	DBG_LOG(DBG_DEBUG, DBG_MODULE(server_bcs), 
	        "SharedBuffer: path = %s, size = %" PRIu64 ", base = %p\n", 
	        path_.c_str(), size_, base_);

	return E_SUCCESS;
}


int 
SharedBuffer::SignalReader()
{
	int ret;
	int r;

	printf("SIGNAL READER\n");
	if ((ret = ipc_->call(::SharedBuffer::Protocol::kConsume, 
                          ipc_->id(), id_, r)) < 0) 
	{
		printf("ERROR\n");
		return -E_IPC;
	}
	if (ret > 0) {
		return -ret;
	}


	return E_SUCCESS;
}


int 
SharedBuffer::Write(const void* src, size_t n)
{
	SignalReader();
}

} // namespace client
