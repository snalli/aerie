#ifndef __STAMNOS_BCS_CLIENT_SHARED_BUFFER_H
#define __STAMNOS_BCS_CLIENT_SHARED_BUFFER_H

#include <stddef.h>
#include <string>
#include "bcs/main/client/bcs-opaque.h"
#include "bcs/main/common/shbuf.h"
#include "common/mmapregion.h"

namespace client {

class SharedBuffer: public MemoryMappedRegion< ::SharedBuffer::Header> {
public:
	SharedBuffer(Ipc* ipc, ::SharedBuffer::Descriptor& dsc)
		: path_(dsc.path_),
		  ipc_(ipc),
		  id_(dsc.id_)
	{ }

	int Map();

	int Write(const void* src, size_t n);

private:
	int SignalReader();
	
	Ipc*        ipc_;
	std::string path_;
	int         id_; // handle given by the server
};

} // namespace client

#endif // __STAMNOS_BCS_CLIENT_SHARED_BUFFER_H
