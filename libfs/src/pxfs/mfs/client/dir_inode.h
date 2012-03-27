#ifndef __STAMNOS_MFS_CLIENT_DIRECTORY_INODE_H
#define __STAMNOS_MFS_CLIENT_DIRECTORY_INODE_H

#include <stdint.h>
#include <google/sparsehash/sparseconfig.h>
#include <google/dense_hash_map>
#include "pxfs/common/types.h"
#include "pxfs/common/const.h"
#include "pxfs/client/inode.h"
#include "ssa/containers/assoc/hashtable.h"
#include "ssa/containers/name/container.h"
#include "ssa/main/common/obj.h"

namespace client {
	class Session; // forward declaration
}

namespace mfs {
namespace client {

class FileInode;

class DirInode: public ::client::Inode
{
friend class FileInode;
public:
	DirInode(ssa::common::ObjectProxyReference* ref)
		: parent_(NULL)
	{ 
		ref_ = ref;
		fs_type_ = ::common::fs::kMFS;
		type_ = ::client::type::kDirInode;
	}

	int Read(::client::Session* session, char* dst, uint64_t off, uint64_t n) { return 0; }
	int Write(::client::Session* session, char* src, uint64_t off, uint64_t n) { return 0; }
	int Unlink(::client::Session* session, const char* name);
	int Lookup(::client::Session* session, const char* name, int flags, ::client::Inode** ipp);
	int xLookup(::client::Session* session, const char* name, int flags, ::client::Inode** ipp);
	int Link(::client::Session* session, const char* name, ::client::Inode* ip, bool overwrite);
	int Link(::client::Session* session, const char* name, uint64_t ino, bool overwrite) { assert(0); }
	int Sync(::client::Session* session);

	int nlink();
	int set_nlink(int nlink);

	int Lock(::client::Session* session, Inode* parent_inode, lock_protocol::Mode mode); 
	int Lock(::client::Session* session, lock_protocol::Mode mode); 
	int Unlock(::client::Session* session);
	int xOpenRO(::client::Session* session); 

	int ioctl(::client::Session* session, int request, void* info);

private:
	ssa::containers::client::NameContainer::Reference* rw_ref() {
		return static_cast<ssa::containers::client::NameContainer::Reference*>(ref_);
	}
	int              nlink_;
	::client::Inode* parent_; // special case; see comment under link
};


} // namespace client
} // namespace mfs

#endif // __STAMNOS_MFS_CLIENT_DIRECTORY_INODE_H
