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
	
	SuperBlock(dpo::containers::client::SuperContainer::Reference rw_ref)
		: super_rw_ref_(rw_ref)
	{ 
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
