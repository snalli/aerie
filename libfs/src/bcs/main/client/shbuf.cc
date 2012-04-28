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
	
	if ((ret = Open(path_.c_str(), 0, kMap, this)) < 0) {
		return ret;
	}

	DBG_LOG(DBG_DEBUG, DBG_MODULE(server_bcs), 
	        "SharedBuffer: path = %s, size = %" PRIu64 ", base = %p\n", 
	        path_.c_str(), size_, (void*) base_);

	return E_SUCCESS;
}


int 
SharedBuffer::SignalReader()
{
	int ret;
	int r;

	if ((ret = ipc_->call(::SharedBuffer::Protocol::kConsume, 
                          ipc_->id(), id_, r)) < 0) 
	{
		return -E_IPC;
	}
	if (ret > 0) {
		return -ret;
	}

	return E_SUCCESS;
}


/**
 * \brief returns the number of bytes written
 */
int 
SharedBuffer::Write(const char* src, size_t n)
{
	if (n >= size() - Count()) {
		// no space 
		SignalReader();
	}
	void* bptr = (void*) (base() + end());
	if (end() >= start()) {
		uint64_t empty_slots = size() - end();
		if (n > empty_slots) {
			memcpy(bptr, src, empty_slots);
			memcpy((void*) base(), &src[empty_slots], n-empty_slots);
		} else {
			memcpy(bptr, src, n);
		}
	} else {
		memcpy(bptr, src, n);
	}
	set_end((end() + n) % size());
	return n;
}

} // namespace client
