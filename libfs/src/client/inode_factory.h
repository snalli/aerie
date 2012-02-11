#ifndef __STAMNOS_CLIENT_INODE_FACTORY_H
#define __STAMNOS_CLIENT_INODE_FACTORY_H

#include "dpo/main/common/obj.h"

namespace client {

class Session;  // forward declaration
class Inode;    // forward declaration 

class InodeFactory {
public:
	virtual int Make(Session* session, int type, Inode** ipp) = 0;
	virtual int Load(Session* session, dpo::common::ObjectId oid, Inode** ipp) = 0;
};

} // namespace client

#endif // __STAMNOS_CLIENT_INODE_FACTORY_H
