#ifndef __STAMNOS_MFS_CLIENT_SUPERBLOCK_H
#define __STAMNOS_MFS_CLIENT_SUPERBLOCK_H

#include <pthread.h>
#include "common/types.h"
#include "pxfs/client/backend.h"
#include "ssa/containers/super/container.h"
#include "ssa/main/common/obj.h"
#include "pxfs/mfs/client/dir_inode.h"

namespace mfs {
namespace client {

class SuperBlockFactory; // forward declaration

class SuperBlock: public ::client::SuperBlock {
friend class SuperBlockFactory;
public:
	SuperBlock(ssa::common::ObjectProxyReference* ref)
		: super_rw_ref_(static_cast<ssa::containers::client::SuperContainer::Reference*>(ref)),
		  root_(NULL)
	{ }

	//static SuperBlock* Load(::client::Session* session, ssa::common::ObjectId oid);
	::client::Inode* RootInode() {
		return root_;
	}
	
	ssa::common::ObjectId oid() {
		return super_rw_ref_->proxy()->oid();	
	}

private:
	ssa::containers::client::SuperContainer::Reference* super_rw_ref_;
	::client::Inode*                                    root_;
};

} // namespace client
} // namespace mfs

#endif // __STAMNOS_MFS_CLIENT_SUPERBLOCK_H
