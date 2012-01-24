#include "mfs/client/sb_factory.h"
#include "common/errno.h"
#include "client/session.h"
#include "dpo/containers/super/container.h"
#include "dpo/containers/name/container.h"
#include "dpo/base/common/obj.h"
#include "mfs/client/sb.h"

namespace mfs {
namespace client {


SuperBlockFactory::SuperBlockFactory()
{
	
}


int 
SuperBlockFactory::Load(::client::Session* session, dpo::common::ObjectId oid, 
                        ::client::SuperBlock** sbp)
{
	int                                                ret;
	SuperBlock*                                        sb; 
	dpo::containers::client::SuperContainer::Reference rw_ref;
	dpo::common::ObjectId                              root_inode_oid;

	if ((ret = session->omgr_->GetObject(oid, &rw_ref)) < 0) {
		//FIXME: deallocate the allocated object
		return ret;
	}

	root_inode_oid = rw_ref.obj()->interface()->root(session);


	sb = new SuperBlock(rw_ref);

	*sbp = sb;
	return E_SUCCESS;
}


int
SuperBlockFactory::Make(::client::Session* session, ::client::SuperBlock** sbp)
{
	int                                                ret;
	SuperBlock*                                        sb; 
	dpo::containers::client::SuperContainer::Object*   super_obj;
	dpo::containers::client::SuperContainer::Proxy*    super_proxy;
	dpo::containers::client::NameContainer::Object*    root_obj;

	// FIXME: allocate object through the storage manager
	if ((super_obj = new(session) dpo::containers::client::SuperContainer::Object) == NULL) {
		return -E_NOMEM;
	}
	if ((root_obj = new(session) dpo::containers::client::NameContainer::Object) == NULL) {
		return -E_NOMEM;
	}
	if ((ret = Load(session, super_obj->oid(), sbp)) < 0) {
		//FIXME: deallocate the allocated object
		return ret;
	}
	//sbp->rw_ref_.obj()->interface()->set_root(session, root_obj->oid());
	return E_SUCCESS;
}


} // namespace client
} // namespace mfs
