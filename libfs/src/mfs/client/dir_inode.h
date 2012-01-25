#ifndef __STAMNOS_MFS_CLIENT_DIRECTORY_INODE_H
#define __STAMNOS_MFS_CLIENT_DIRECTORY_INODE_H

#include <stdint.h>
#include <google/sparsehash/sparseconfig.h>
#include <google/dense_hash_map>
#include "common/types.h"
#include "common/const.h"
#include "client/inode.h"
#include "mfs/hashtable.h"
#include "dpo/containers/name/container.h"
#include "dpo/base/common/obj.h"

namespace client {
	class Session; // forward declaration
}

namespace mfs {
namespace client {

class DirInode: public ::client::Inode
{
public:
	DirInode(dpo::common::ObjectProxyReference* ref)
	{ 
		ref_ = ref;
		fs_type_ = ::common::fs::kMFS;
		type_ = ::client::type::kDirInode;
	}

	//static DirInode* Load(::client::Session* session, dpo::common::ObjectId oid);

	int Open(::client::Session* session, const char* path, int flags) { return 0; };
	int Read(::client::Session* session, char* dst, uint64_t off, uint64_t n) { return 0; }
	int Write(::client::Session* session, char* src, uint64_t off, uint64_t n) { return 0; }
	int Unlink(::client::Session* session, const char* name);
	int Lookup(::client::Session* session, const char* name, ::client::Inode** ipp);
	int Link(::client::Session* session, const char* name, ::client::Inode* ip, bool overwrite);
	int Link(::client::Session* session, const char* name, uint64_t ino, bool overwrite);
	int Publish(::client::Session* session);

	int nlink();
	int set_nlink(int nlink);

private:
	dpo::containers::client::NameContainer::Reference* rw_ref() {
		return static_cast<dpo::containers::client::NameContainer::Reference*>(ref_);
	}
	int nlink_;
};


} // namespace client
} // namespace mfs

#endif // __STAMNOS_MFS_CLIENT_DIRECTORY_INODE_H
