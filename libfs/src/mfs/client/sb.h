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


class SuperBlock: public ::client::SuperBlock {
public:
	::client::Inode* RootInode() {
		return root_;
	}
	
	SuperBlock()
		: super_rw_ref_(this),
		  root_(NULL)
	{ }

/*
	static SuperBlock* Load(::client::Session* session, dpo::common::ObjectId oid) {
		SuperBlock*           sbp;
		dpo::common::ObjectId root_inode_oid;

		sbp = new SuperBlock();
		if (session->omgr_->GetObject(oid, &(sbp->super_rw_ref_)) < 0) {
			delete sbp;
			return NULL;
		}
		printf("DONE 1\n");
		root_inode_oid = sbp->super_rw_ref_.obj()->interface()->root(session);
		if (root_inode_oid.u64() != 0) {
			sbp->root_ = DirInode::Load(session, root_inode_oid);
			assert(sbp->root_ != NULL); 
		}
		printf("DONE 2\n");
		
		return sbp;
	}
*/

	static SuperBlock* Load(::client::Session* session, dpo::common::ObjectId oid) {

	}

	dpo::common::ObjectId oid() {
		return super_rw_ref_.obj()->oid();	
	}

private:
	dpo::containers::client::SuperContainer::Reference super_rw_ref_;
	::client::Inode*                                   root_;
};

} // namespace client
} // namespace mfs

#endif // __STAMNOS_MFS_CLIENT_SUPERBLOCK_H
