#include "mfs/client/sb_factory.h"
#include "common/errno.h"
#include "client/session.h"
#include "dpo/containers/super/container.h"
#include "mfs/client/sb.h"

namespace mfs {
namespace client {


SuperBlockFactory::SuperBlockFactory()
{
	
}


int
SuperBlockFactory::Make(::client::Session* session, ::client::SuperBlock** sbp)
{
	int                                                ret;
	SuperBlock*                                        sb; 
	dpo::containers::client::SuperContainer::Object*   obj;
	dpo::containers::client::SuperContainer::Reference rw_ref;

	// FIXME: allocate object through the storage manager
	if ((obj = new(session) dpo::containers::client::SuperContainer::Object) == NULL) {
		return -E_NOMEM;
	}
	if ((ret = session->omgr_->GetObject(obj->oid(), &rw_ref)) < 0) {
		//FIXME: deallocate the allocated object
		return ret;
	}
	sb = new SuperBlock(rw_ref);

	*sbp = sb;
	return E_SUCCESS;
}


int 
SuperBlockFactory::Load(::client::Session* session, dpo::common::ObjectId oid, 
                        ::client::SuperBlock** sbp)
{
	int                                                ret;
	SuperBlock*                                        sb; 
	dpo::containers::client::SuperContainer::Reference rw_ref;

	if ((ret = session->omgr_->GetObject(oid, &rw_ref)) < 0) {
		//FIXME: deallocate the allocated object
		return ret;
	}
	sb = new SuperBlock(rw_ref);

	*sbp = sb;
	return E_SUCCESS;
}


} // namespace client
} // namespace mfs
