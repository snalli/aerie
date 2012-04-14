#ifndef __STAMNOS_CFS_SERVER_NAMESPACE_H
#define __STAMNOS_CFS_SERVER_NAMESPACE_H

#include <sys/types.h>
#include "cfs/common/types.h"

namespace server {

class Session;

class NameSpace {
public:
	NameSpace(InodeNumber root_ino)
		: root_ino_(root_ino),
		  cwd_(root_ino)
	{ }

	int Nameiparent(Session* session, const char* path, char* name, InodeNumber* ino);
	int Namei(Session* session, const char* path, InodeNumber* ino);
	int Init(Session* session);

	InodeNumber root_ino() { return root_ino_; }

private:
	int Namex(Session* session, const char *cpath, bool nameiparent, char* name, InodeNumber* ino);

	InodeNumber root_ino_;
	InodeNumber cwd_;
};

} // namespace server

#endif // __STAMNOS_CFS_SERVER_NAMESPACE_H
