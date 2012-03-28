#include "ssa/main/server/verifier.h"
#include "ssa/main/server/lckmgr.h"
#include "ssa/main/server/session.h"
#include "ssa/main/server/stsystem.h"
#include "common/errno.h"

namespace server {

int 
LockVerifier::VerifyLock(::ssa::server::SsaSession* session, ssa::common::ObjectId oid)
{
	ssa::common::Object*  next_obj;
	ssa::common::ObjectId next = oid;
	
	while(true) {
		next_obj = ssa::common::Object::Load(next);
		ssa::cc::common::LockId lid(1, next.num());
		lock_protocol::Mode mode = session->storage_system_->hlckmgr()->LockMode(session->clt(), lid.u64());
		if (mode == lock_protocol::Mode::NONE) {
			if ((next = next_obj->parent()) == ssa::common::ObjectId(0)) {
				return -1;
			}
		} else {
			break;
		}
	} 
	return E_SUCCESS;
}

} // namespace server
