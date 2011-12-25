#ifndef __MFS_CLIENT_DIRECTORY_INODE_H_KAL178
#define __MFS_CLIENT_DIRECTORY_INODE_H_KAL178

#include <stdint.h>
#include <google/sparsehash/sparseconfig.h>
#include <google/dense_hash_map>
#include "common/types.h"
#include "client/sb.h"
#include "client/inode.h"
#include "mfs/hashtable.h"
#include "mfs/dir_pnode.h"

namespace mfs {

// we keep negative entries to indicate absence of the entry
// (in contrast to the DLNC in Solaris and FreeBSD where the negative entry
//  is used as a performance optimization, the negative entry in this system 
//  is necessary for correctness)

class DirInodeMutable: public client::Inode
                       //public cow::ObjectProxy<DirInodeMutable, DirPnode<client::Session> >  
{
	//typedef cow::ObjectProxy<DirInodeMutable, DirPnode<client::Session> > ObjectProxy;

public:
	typedef google::dense_hash_map<std::string, std::pair<bool, uint64_t> > EntryCache;
	
	DirInodeMutable(client::SuperBlock* sb, Pnode* pnode)
		: Inode(sb, pnode, reinterpret_cast<InodeNumber>(pnode)),
		  neg_entries_count_(0)
	{ 
		//FIXME: ObjectProxy::setSubject(reinterpret_cast<DirPnode<client::Session>*>(pnode_));
		nlink_ = pnode_->nlink_;
		entries_.set_empty_key("");
		printf("DirInodeMutable: %p\n", this);
		printf("DirInodeMutable: pnode=%p\n", pnode);
		printf("DirInodeMutable: pnode->nlink_=%d\n", pnode->nlink_);
		printf("DirInodeMutable: ino=%lu\n", ino_);
		printf("DirInodeMutable: nlink_=%d\n", nlink_);
	}

	int Init(client::Session* session, uint64_t ino) {
		ino_ = ino;
		pnode_ = DirPnode<client::Session>::Load(ino);
		printf("DirInodeMutable: ino=%lu\n", ino);
		printf("DirInodeMutable: pnode_=%p\n", pnode_);
		return 0;
	}
	int Open(client::Session* session, const char* path, int flags) { };
	int Read(client::Session* session, char* dst, uint64_t off, uint64_t n) { return 0; }
	int Write(client::Session* session, char* src, uint64_t off, uint64_t n) { return 0; }
	int Insert(client::Session* session, const char* name, client::Inode* inode) { };
	int Unlink(client::Session* session, const char* name);

	int Lookup(client::Session* session, const char* name, client::Inode** ipp);

	int Link(client::Session* session, const char* name, client::Inode* ip, bool overwrite);
	int Link(client::Session* session, const char* name, uint64_t ino, bool overwrite);
	int Publish(client::Session* session);

	int nlink();
	int set_nlink(int nlink);

private:
	//DirPnode<client::Session>* dir_pnode();
	
	EntryCache                  entries_;
	int                         neg_entries_count_; // number of negative entries in the map entries_
};

/*
inline DirPnode<client::Session>*  
DirInodeMutable::dir_pnode() {
	return reinterpret_cast<DirPnode<client::Session>*>(pnode_);
}
*/

} // namespace mfs

#endif // __MFS_CLIENT_DIRECTORY_INODE_H_KAL178
