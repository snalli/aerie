#ifndef __STAMNOS_OSD_CLIENT_JOURNAL_H
#define __STAMNOS_OSD_CLIENT_JOURNAL_H

#include <stdio.h>
#include <stdint.h>
#include "osd/main/client/osd-opaque.h"
#include "osd/main/client/shbuf.h"
#include "osd/main/common/publisher.h"
#include "osd/main/common/journal.h"


namespace osd {
namespace client {

class Buffer {
public:
	Buffer() 
		: size_(128*1024),
		  count_(0)
	{ }

	inline int Write(const void* src, size_t n);
	inline int Flush(OsdSharedBuffer* dst);

private:
	char   buf_[128*1024];
	size_t size_;
	size_t count_;
};


int
Buffer::Write(const void* src, size_t n)
{
	memcpy(&buf_[count_], src, n);
	count_ += n;
	if (count_ > size_) {
		dbg_log(DBG_CRITICAL, "Run out of journal space\n");
	}
	return E_SUCCESS;
}


int
Buffer::Flush(OsdSharedBuffer* dst)
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
	Journal(OsdSession* session)
		: session_(session),
		  mode_(osd::common::Journal::Client)
	{ }

	int TransactionBegin(int id = 0);
	int TransactionCommit();
	int Write(const void* buf, size_t size) {
		return buffer_.Write(buf, size);
	}

	template<typename T>
	void Store(volatile T* addr, T val)
	{
		*addr = val;
	}

	inline friend Journal* operator<< (Journal* journal, const osd::Publisher::Message::ContainerOperationHeader& header) {
		journal->Write(&header, sizeof(header) + header.payload_size_);
		return journal;
    }
	
	inline friend Journal* operator<< (Journal* journal, const osd::Publisher::Message::LogicalOperationHeader& header) {
		journal->Write(&header, sizeof(header) + header.payload_size_);
		return journal;
    }

	int mode() { return mode_; }
private:
	OsdSession* session_;
	Buffer      buffer_;
	int         mode_; // indicates whether client or server mode
};


} // namespace client
} // namespace osd

#endif // __STAMNOS_OSD_CLIENT_JOURNAL_H
