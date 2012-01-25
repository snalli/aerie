#ifndef __STAMNOS_MFS_CLIENT_DIRECTORY_INODE_H
#define __STAMNOS_MFS_CLIENT_DIRECTORY_INODE_H

#include <stdint.h>
#include <google/sparsehash/sparseconfig.h>
#include <google/dense_hash_map>
#include "common/types.h"
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
		: rw_ref_(static_cast<dpo::containers::client::NameContainer::Reference*>(ref))
	{ }

/*
	static DirInode* Load(::client::Session* session, dpo::common::ObjectId oid) {
		DirInode*  dip;

		dip = new DirInode();
		if (session->omgr_->GetObject(oid, &(dip->rw_ref_)) < 0) {
			delete dip;
			return NULL;
		}
		return dip;
	}

	static DirInode* Load(::client::Session* session, dpo::common::ObjectId oid) {
		DirInode*  dip;

		if (session->omgr_->FindOrGetObject(oid, &(dip->rw_ref_)) < 0) {
			delete dip;
			return NULL;
		}
		dip = new DirInode();
		return dip;
	}
*/


	dpo::common::ObjectId oid() {
		return rw_ref_->obj()->oid();	
	}

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
	dpo::containers::client::NameContainer::Reference* rw_ref_;
};


} // namespace client
} // namespace mfs

#endif // __STAMNOS_MFS_CLIENT_DIRECTORY_INODE_H
