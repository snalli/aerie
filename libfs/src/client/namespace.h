#ifndef _NAMESPACE_H_AGT127
#define _NAMESPACE_H_AGT127

#include <sys/types.h>
#include "rpc/rpc.h"
#include "client/mpinode.h"
#include "client/sb.h"

namespace client {

class NameSpace {
public:
	NameSpace(rpcc* c, unsigned int, const char*);
	int Lookup(const char* name, void**);
	int Link(const char* name, void*);
	int Unlink(const char* name);
	int Namei(const char* path, Inode**);
	int Nameiparent(const char* path, char* name, Inode**);
	int Mount(char*, SuperBlock*);
	int Unmount(char*);
	int Init();

private:
	int Namex(const char* path, bool nameiparent, char* name, Inode**);

	rpcc*        client_;
	unsigned int principal_id_;
	char         namespace_name_[128];
	MPInode*     root_;
};

} // namespace client

#endif
