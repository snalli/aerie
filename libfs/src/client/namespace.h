#ifndef _NAMESPACE_H_AGT127
#define _NAMESPACE_H_AGT127

#include <sys/types.h>
#include "rpc/rpc.h"
#include "client/mpinode.h"
#include "client/sb.h"

namespace client {

class Session;

class NameSpace {
public:
	NameSpace(rpcc* c, unsigned int, const char*);
	int Lookup(Session* session, const char* name, void**);
	int Link(Session* session, const char* oldpath, const char* newpath);
	int Unlink(Session* session, const char* pathname);
	int Namei(Session* session, const char* path, lock_protocol::Mode lock_mode, Inode**);
	int Nameiparent(Session* session, const char* path, lock_protocol::Mode lock_mode, char* name, Inode**);
	int Mount(Session* session, const char*, SuperBlock*);
	int Unmount(Session* session, char*);
	int Init(Session* session);

private:
	int LockInodeReverse(Session* session, Inode* inode, lock_protocol::Mode lock_mode);
	int Namex(Session* session, const char* path, lock_protocol::Mode lock_mode, bool nameiparent, char* name, Inode**);

	rpcc*        client_;
	unsigned int principal_id_;
	char         namespace_name_[128];
	MPInode*     root_;
};

} // namespace client

#endif
