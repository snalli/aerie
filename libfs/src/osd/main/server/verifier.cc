#include "osd/main/server/verifier.h"
#include "osd/main/server/lckmgr.h"
#include "osd/main/server/session.h"
#include "osd/main/server/stsystem.h"
#include "common/errno.h"

namespace server {

int 
LockVerifier::VerifyLock(::osd::server::OsdSession* session, osd::common::ObjectId oid)
{
	osd::common::Object*  next_obj;
	osd::common::ObjectId next = oid;
	
	while(true) {
		next_obj = osd::common::Object::Load(next);
		osd::cc::common::LockId lid(1, next.num());
		lock_protocol::Mode mode = session->storage_system_->hlckmgr()->LockMode(session->clt(), lid.u64());
		if (mode == lock_protocol::Mode::NONE) {
			if ((next = next_obj->parent()) == osd::common::ObjectId(0)) {
				return -1;
			}
		} else {
			break;
		}
	} 
	return E_SUCCESS;
}

} // namespace server
