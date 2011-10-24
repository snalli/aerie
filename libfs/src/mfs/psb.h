#ifndef _PERSISTENT_SUPERBLOCK_MFS_H_JAK129
#define _PERSISTENT_SUPERBLOCK_MFS_H_JAK129

#include <stdint.h>
#include <typeinfo>
#include "client/backend.h"
#include "mfs/mfs_i.h"
#include "mfs/hashtable.h"
#include "common/debug.h"

// The persistent structures stored in SCM
//
// FIXME: 'new' operator should take as argument a transaction id that passes
// down to the storage allocator to ensure atomicity of the allocation

// FIXME: functions that write SCM must take a journal as argument 

namespace mfs {

template<typename Session>
class DirPnode;

template<typename Session>
class PSuperBlock {
public:
	PSuperBlock() 
		: magic_(mfs::magic::kSuperBlock)
	{ }

	static PSuperBlock* Load(uint64_t ptr) {
		PSuperBlock* psb = reinterpret_cast<PSuperBlock*>(ptr);

		// check whether type is the expected one
		if (psb->magic_ == mfs::magic::kSuperBlock) {
			return psb;
		}
		return NULL;
	}

	void* operator new(size_t nbytes, Session* session)
	{
		void* ptr;
		int   ret;

		if ((ret = session->sm->Alloc(session, nbytes, typeid(PSuperBlock), &ptr)) < 0) {
			dbg_log(DBG_ERROR, "No storage available");
		}
		return ptr;
	}

	uint64_t root_;
	uint32_t magic_;
};


} // namespace mfs


#endif // _PERSISTENT_SUPERBLOCK_MFS_H_JAK129
