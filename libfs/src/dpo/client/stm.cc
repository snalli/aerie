#include "dpo/client/stm.h"

namespace client {

namespace stm {

__thread Transaction* transaction_; // per thread transaction descriptor


int
Transaction::Init()
{
	//FIXME
	//nesting_ = 0;
	//rset_.set_empty_key(0);
}

/*

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
Transaction::OpenRO(Object* obj)
{
	ReadSet::iterator it;

	if ((it = rset_.find(obj)) != rset_.end()) {
		return 0; 
	}

	ReadSetEntry rset_entry;
	rset_entry.version_ = obj->xVersion();
	rset_[obj] = rset_entry;
	return 0;
}


int 
Transaction::Validate()
{
	ReadSet::iterator it;
	Object*           obj;

	for (it = rset_.begin(); it != rset_.end(); it++) {
		obj = it->first;
		if (obj->xVersion() > it->second.version_) {
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
*/

} // namespace stm

} // namespace client
