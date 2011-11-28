//! \file
//! Definition of the persistent inode structure, whichs forms a base class 
//! for persistent inode structures
//!

#ifndef __PNODE_STAMNOS_H_KAL189
#define __PNODE_STAMNOS_H_KAL189

#include "common/stm.h"
#include "common/types.h"


class Pnode: public common::stm::Object {
public:
	static Pnode* Load(uint64_t ino) {
		return reinterpret_cast<Pnode*>(ino);
	}
	
	InodeNumber ino() { return ino_; }

	uint32_t    magic_;
	InodeNumber ino_;
	uint32_t    nlink_;
};

#endif /* __PNODE_STAMNOS_H_KAL189 */
