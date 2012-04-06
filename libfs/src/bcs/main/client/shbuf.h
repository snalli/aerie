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
		: ipc_(ipc),
		  path_(dsc.path_),
		  id_(dsc.id_)
	{ }

	uint64_t size() { return header_->payload_size(); }
	uint64_t base() { return header_->payload_base(); }
	uint64_t start() { return header_->start_; }
	uint64_t end() { return header_->end_; }
	void set_end(uint64_t end) { header_->end_ = end; }
	
	int Map();

	int Write(const char* src, size_t n);
	
	int Count() {
		return (size() + end() - start()) % size();
	}
	int SignalReader();
protected:
	
	Ipc*        ipc_;
	std::string path_;
	int         id_; // handle given by the server
};

} // namespace client

#endif // __STAMNOS_BCS_CLIENT_SHARED_BUFFER_H
