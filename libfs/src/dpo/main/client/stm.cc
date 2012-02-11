#include "dpo/main/client/stm.h"

namespace dpo {

namespace stm {

namespace client {

__thread Transaction* transaction_; // per thread transaction descriptor


int
Transaction::Init()
{
	nesting_ = 0;
	rset_.set_empty_key(0);
}


int 
Transaction::Start(JmpBuf* jmpbuf, uint32_t abort_flags)
{
	if (abort_flags == ABORT_EXPLICIT) {
		return -1;
	}

	if (nesting_++ > 0) {
		return 0;
	}

	return 0;
}


int 
Transaction::Commit()
{
	if (Validate() < 0) {
		Rollback(ABORT_VALIDATE);
	}
	return 0;
}


int
Transaction::OpenRO(::dpo::cc::common::Object* obj)
{
	ReadSet::iterator it;

	if ((it = rset_.find(obj)) != rset_.end()) {
		return 0; 
	}

	ReadSetEntry rset_entry;
	rset_entry.version_ = obj->ccVersion();
	rset_[obj] = rset_entry;
	return 0;
}


int 
Transaction::Validate()
{
	ReadSet::iterator          it;
	::dpo::cc::common::Object* obj;

	for (it = rset_.begin(); it != rset_.end(); it++) {
		obj = it->first;
		if (obj->ccVersion() > it->second.version_) {
			return -1;
		}
	}
	return 0;
}


void 
Transaction::AbortIfInvalid()
{
	if (Validate() < 0) {
		Rollback(ABORT_VALIDATE);
	}
}


void 
Transaction::Abort()
{
	Rollback(ABORT_EXPLICIT);
}


void 
Transaction::Rollback(int flags)
{
	nesting_ = 0;
	rset_.clear();

	siglongjmp(jmpbuf_, flags);
}

} // namespace client
} // namespace stm
} // namespace dpo
