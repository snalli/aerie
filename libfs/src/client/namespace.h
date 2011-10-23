#ifndef _NAMESPACE_H_AGT127
#define _NAMESPACE_H_AGT127

#include <sys/types.h>
#include "rpc/rpc.h"
#include "client/mpinode.h"
#include "client/sb.h"

namespace client {

class ClientSession;

class NameSpace {
public:
	NameSpace(rpcc* c, unsigned int, const char*);
	int Lookup(ClientSession* session, const char* name, void**);
	int Link(ClientSession* session, const char* name, void*);
	int Unlink(ClientSession* session, const char* name);
	int Namei(ClientSession* session, const char* path, Inode**);
	int Nameiparent(ClientSession* session, const char* path, char* name, Inode**);
	int Mount(ClientSession* session, const char*, SuperBlock*);
	int Unmount(ClientSession* session, char*);
	int Init(ClientSession* session);

private:
	int Namex(ClientSession* session, const char* path, bool nameiparent, char* name, Inode**);

	rpcc*        client_;
	unsigned int principal_id_;
	char         namespace_name_[128];
	MPInode*     root_;
};

} // namespace client

#endif
