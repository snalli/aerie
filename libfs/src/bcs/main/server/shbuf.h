#ifndef __STAMNOS_BCS_SERVER_SHARED_BUFFER_H
#define __STAMNOS_BCS_SERVER_SHARED_BUFFER_H

#include <stddef.h>
#include <string>
#include "bcs/main/common/shbuf.h"
#include "common/mmapregion.h"

namespace server {

class SharedBuffer: public MemoryMappedRegion< ::SharedBuffer::Header> {
public:
	int Init(const char* suffix);
	::SharedBuffer::Descriptor Descriptor() {
		return ::SharedBuffer::Descriptor(id_, path_, size_);
	}

	virtual int Consume();

//private:
	int         id_; // an identifier local to the client assigned the buffer
	std::string path_;
};


} // namespace server

#endif // __STAMNOS_BCS_SERVER_SHARED_BUFFER_H
