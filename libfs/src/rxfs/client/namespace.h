#ifndef __STAMNOS_RXFS_CLIENT_NAMESPACE_H
#define __STAMNOS_RXFS_CLIENT_NAMESPACE_H

#include <sys/types.h>
#include "rxfs/common/types.h"
#include "osd/main/client/osd.h"
#include "osd/main/client/hlckmgr.h"

namespace rxfs {
namespace client {

class Session;

class NameSpace {
public:
	NameSpace(InodeNumber root_ino)
		: root_ino_(root_ino),
		  cwd_(root_ino)
	{ }

	int Nameiparent(Session* session, const char* path, char* name, lock_protocol::Mode mode, InodeNumber* ino);
	int Namei(Session* session, const char* path, lock_protocol::Mode mode, InodeNumber* ino);
	int Init(Session* session);

	InodeNumber root_ino() { return root_ino_; }

private:
	int Namex(Session* session, const char *cpath, bool nameiparent, char* name, InodeNumber* ino);

	InodeNumber root_ino_;
	InodeNumber cwd_;
};

} // namespace client
} // namespace rxfs

#endif // __STAMNOS_RXFS_CLIENT_NAMESPACE_H
