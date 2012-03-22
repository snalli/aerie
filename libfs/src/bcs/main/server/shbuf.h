#ifndef __STAMNOS_BCS_SERVER_SHARED_BUFFER_H
#define __STAMNOS_BCS_SERVER_SHARED_BUFFER_H

#include <stddef.h>
#include <string>
#include "bcs/main/server/bcs-opaque.h"
#include "bcs/main/common/shbuf.h"
#include "common/mmapregion.h"

namespace server {

class SharedBuffer: public MemoryMappedRegion< ::SharedBuffer::Header> {
public:
	int Init(const char* suffix);
	::SharedBuffer::Descriptor Descriptor() {
		return ::SharedBuffer::Descriptor(id_, path_, size_);
	}

	virtual int Consume(BcsSession* session) = 0;
	void set_id(int id) { id_ = id; };
	/**
	 * \brief Similarly to Acquire/Release in multiprocessor consistency models, 
	 * we snapshot the end-pointer of the buffer because we don't trust the 
	 * client won't change it after we read it. 
	 */
	void Acquire()	{
		end_ = header_->end_;
	}
	void Release() {
		header_->start_ = start_;
	}
	int Count() {
		return (size_ + end_ - start_) % size_;
	}

	int Read(char* dst, size_t n);

//private:
	int         id_;    // an identifier local to the client assigned the buffer
	std::string path_;
	size_t      start_; // shapshot of the buffer start-pointer
	size_t      end_;   // snapshot of the buffer end-pointer
};


} // namespace server

#endif // __STAMNOS_BCS_SERVER_SHARED_BUFFER_H
