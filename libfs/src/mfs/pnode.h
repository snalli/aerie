//! \file
//! Definition of the persistent inode structure, whichs forms a base class 
//! for persistent inode structures
//!

#ifndef __PINODE_H_KAL189
#define __PINODE_H_KAL189

namespace mfs {

class Pnode {
public:
	static Pnode* Load(uint64_t ino) {
		return reinterpret_cast<Pnode*>(ino);
	}

	uint32_t   magic_;
	uint64_t   ino_;
	uint64_t   gen_;
	uint32_t   nlink_;
};

} // namespace mfs

#endif /* __PINODE_H_KAL189 */
