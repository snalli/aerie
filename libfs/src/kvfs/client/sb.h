#ifndef __STAMNOS_KVFS_CLIENT_SB_H
#define __STAMNOS_KVFS_CLIENT_SB_H


#include "osd/containers/super/container.h"
#include "osd/containers/name/container.h"
#include "kvfs/client/table.h"
#include "kvfs/client/session.h"


namespace client {

class SuperBlock {
public:
	SuperBlock(osd::common::ObjectProxyReference* ref)
		: super_rw_ref_(static_cast<osd::containers::client::SuperContainer::Reference*>(ref)),
		  tp_(NULL)
	{ }

	static int Load(::client::Session* session, osd::common::ObjectId oid, ::client::SuperBlock** sbp);
	
	
	::client::Table* Table() {
		return tp_;
	}
	
	osd::common::ObjectId oid() {
		return super_rw_ref_->proxy()->oid();	
	}

private:
	osd::containers::client::SuperContainer::Reference* super_rw_ref_;
	::client::Table*                                    tp_;
};

} // namespace client


#endif // __STAMNOS_KVFS_CLIENT_SB_H
