#ifndef __STAMNOS_MFS_CLIENT_SUPERBLOCK_H
#define __STAMNOS_MFS_CLIENT_SUPERBLOCK_H

#include <pthread.h>
#include "common/types.h"
#include "client/backend.h"
#include "dpo/containers/super/container.h"
#include "dpo/base/common/obj.h"
#include "mfs/client/dir_inode.h"

namespace mfs {
namespace client {

class SuperBlockFactory; // forward declaration

class SuperBlock: public ::client::SuperBlock {
friend class SuperBlockFactory;
public:
	SuperBlock(dpo::common::ObjectProxyReference* ref)
		: super_rw_ref_(static_cast<dpo::containers::client::SuperContainer::Reference*>(ref)),
		  root_(NULL)
	{ }

	//static SuperBlock* Load(::client::Session* session, dpo::common::ObjectId oid);
	::client::Inode* RootInode() {
		return root_;
	}
	
	dpo::common::ObjectId oid() {
		return super_rw_ref_->proxy()->oid();	
	}

private:
	dpo::containers::client::SuperContainer::Reference* super_rw_ref_;
	::client::Inode*                                    root_;
};

} // namespace client
} // namespace mfs

#endif // __STAMNOS_MFS_CLIENT_SUPERBLOCK_H
