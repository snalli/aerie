#ifndef __STAMNOS_SSA_CLIENT_JOURNAL_H
#define __STAMNOS_SSA_CLIENT_JOURNAL_H

#include <stdint.h>
#include "ssa/main/client/ssa-opaque.h"
#include "ssa/main/client/shbuf.h"


namespace ssa {
namespace client {

class Buffer {
public:
	Buffer() 
		: size_(1024),
		  count_(0)
	{ }

	inline int Write(const void* src, size_t n);
	inline int Write64(const uint64_t* src);
	inline int Flush(SsaSharedBuffer* dst);

private:
	char   buf_[1024];
	size_t count_;
	size_t size_;
};

int
Buffer::Write(const void* src, size_t n)
{
	memcpy(&buf_[count_], src, n);
	count_ += n;
	return E_SUCCESS;
}


int 
Buffer::Write64(const uint64_t* src)
{
	uint64_t* dst = (uint64_t*) &buf_[count_];
	*dst = *src;
	count_ += 8;
	return E_SUCCESS;
}


int
Buffer::Flush(SsaSharedBuffer* dst)
{
	int ret;

	if ((ret = dst->Write(buf_, count_)) < 0) {
		return ret;
	}
	count_ = 0;
	return E_SUCCESS;
}


class Journal {
public:
	Journal(SsaSession* session)
		: session_(session)
	{ }

	int BeginLogicalOperation(int id);
	int EndLogicalOperation();
	
private:
	SsaSession* session_;
	Buffer      buffer_;
};


} // namespace client
} // namespace ssa

#endif // __STAMNOS_SSA_CLIENT_JOURNAL_H
