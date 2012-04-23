#ifndef __STAMNOS_OSD_SERVER_JOURNAL_H
#define __STAMNOS_OSD_SERVER_JOURNAL_H

#include "osd/main/common/journal.h"
#include "osd/main/common/publisher.h"

/**
 *  TODO Currently the journal provides just a performance model.
 *       Our prototype does not implement recovery.
 */

namespace osd {
namespace server {

class Journal {
public:
	Journal()
	  : mode_(osd::common::Journal::Server)
	{ }

	int TransactionBegin(int id = 0);
	int TransactionCommit();
	int TransactionAbort();

	template<typename T>
	void Store(volatile T* addr, T val)
	{
		//*addr = val;
	}
	
	// FIXME: This is the Client Journal API that Server Journal does not 
	// support (doesn't make sense to log publishing at Server side).
	// Unfortunately we need these dummy functions to avoid getting compilation problems 
	// of containers at the server. We need a more elegant way to reuse container common.h
	inline friend Journal* operator<< (Journal* journal, const osd::Publisher::Message::ContainerOperationHeader& header) {
		assert(0); // do not use this interface at server side.
    }
	
	inline friend Journal* operator<< (Journal* journal, const osd::Publisher::Message::LogicalOperationHeader& header) {
		assert(0); // do not use this interface at server side.
    }

	int mode() { return mode_; }
private:
	int mode_;
};


} // namespace server
} // namespace osd

#endif // __STAMNOS_OSD_SERVER_JOURNAL_H
