#ifndef __MFS_CLIENT_DIRECTORY_INODE_H_KAL178
#define __MFS_CLIENT_DIRECTORY_INODE_H_KAL178

#include <stdint.h>
#include <google/sparsehash/sparseconfig.h>
#include <google/dense_hash_map>
#include "client/sb.h"
#include "client/inode.h"
#include "mfs/hashtable.h"
#include "mfs/dir_pnode.h"

namespace mfs {

//TODO: DirInodeImmutable: enables faster access for read-only operations.
// does an optimistic lookup and puts the result in inode.
// If a mutable version of the mutable exists then this function
// should fail and ask caller to do the normal slow path lookup

class DirInodeImmutable: public client::Inode {
public:

	DirInodeImmutable(Pnode* pnode)
		: pnode_(static_cast<DirPnode<client::Session>*>(pnode))
	{ 
		pthread_mutex_init(&mutex_, NULL);
	}

	int Init(client::Session* session, uint64_t ino) {
		pthread_mutex_init(&mutex_, NULL);
		pnode_ = DirPnode<client::Session>::Load(ino);
		return 0;
	}
	
	//DirInodeImmutable* Load(uint64_t ino) {	return DirInodeImmutable::Load(InodeImmutable::ino2obj(ino)); }
	//DirInodeImmutable* Load(InodeImmutable* inode) { return reinterpret_cast<DirInodeImmutable*>(inode); }

	int Open(client::Session* session, const char* path, int flags) { assert(0); };
	int Write(client::Session* session, char* src, uint64_t off, uint64_t n) { assert(0); }
	int Read(client::Session* session, char* dst, uint64_t off, uint64_t n) { assert(0); }
	int Lookup(client::Session* session, const char* name, client::Inode** inode) { assert(0); }
	int LookupFast(client::Session* session, const char* name, client::Inode* inode) { assert(0); }
	int Insert(client::Session* session, const char* name, client::Inode* inode) { assert(0); }
	int Link(client::Session* session, const char* name, client::Inode* ip, bool overwrite) { assert(0); }
	int Link(client::Session* session, const char* name, uint64_t ino, bool overwrite) { assert(0); }
	int Unlink(client::Session* session, const char* name) { assert(0); }
	int Publish(client::Session* session) { assert(0); }

	client::SuperBlock* GetSuperBlock() { return sb_;}
	void SetSuperBlock(client::SuperBlock* sb) {sb_ = sb;}

	uint64_t get_ino() {
		return (uint64_t) pnode_;
	}
	
private:
	DirPnode<client::Session>* pnode_;
	client::SuperBlock*        sb_;        // file system superblock

};



// we keep negative entries to indicate absence of the entry
// (in contrast to the DLNC in Solaris and FreeBSD, the 
//  negative entry is necessary for correctness)

class DirInodeMutable: public client::Inode {
public:
	typedef google::dense_hash_map<std::string, std::pair<bool, uint64_t> > EntryCache;
	
	DirInodeMutable(client::SuperBlock* sb, Pnode* pnode)
		: pnode_(static_cast<DirPnode<client::Session>*>(pnode)),
		  sb_(sb),
		  neg_entries_count_(0)
	{ 
		ino_ = (uint64_t) pnode;
		pthread_mutex_init(&mutex_, NULL);
		entries_.set_empty_key("");
		printf("DirInodeMutable: pnode=%p\n", pnode);
		printf("DirInodeMutable: ino=%lu\n", ino_);
	}

	DirInodeMutable(Pnode* pnode)
		: pnode_(static_cast<DirPnode<client::Session>*>(pnode))
	{ assert(0); }


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

	client::SuperBlock* GetSuperBlock() { return sb_;}
	void SetSuperBlock(client::SuperBlock* sb) {sb_ = sb;}
	
	int Lookup(client::Session* session, const char* name, client::Inode** inode);
	int LookupFast(client::Session* session, const char* name, client::Inode* inode) { };

	int Link(client::Session* session, const char* name, client::Inode* ip, bool overwrite);
	int Link(client::Session* session, const char* name, uint64_t ino, bool overwrite);
	int Publish(client::Session* session);

private:
	DirPnode<client::Session>*  pnode_;             // immutable persistent inode structure
	client::SuperBlock*         sb_;                // volatile file system superblock
	EntryCache                  entries_;
	int                         neg_entries_count_; // number of negative entries in the map entries_
};


} // namespace mfs

#endif // __MFS_CLIENT_DIRECTORY_INODE_H_KAL178
