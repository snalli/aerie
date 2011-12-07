//! \file
//! Definition of the persistent inode structure, whichs forms a base class 
//! for persistent inode structures
//!

#ifndef __PNODE_STAMNOS_H_KAL189
#define __PNODE_STAMNOS_H_KAL189

#include "dcc/common/stm.h"
#include "common/types.h"


class Pnode: public dcc::stm::Object {
public:
	static Pnode* Load(uint64_t ino) {
		return reinterpret_cast<Pnode*>(ino);
	}
	
	InodeNumber ino() { return ino_; }
	void set_nlink(uint32_t n) { nlink_ = n; }
	uint32_t nlink() { return nlink_; }

	uint32_t    magic_;
	InodeNumber ino_;
	uint32_t    nlink_;
};


#endif /* __PNODE_STAMNOS_H_KAL189 */
