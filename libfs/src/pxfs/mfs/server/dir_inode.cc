#include "pxfs/mfs/server/dir_inode.h"
#include "pxfs/server/session.h"
#include "pxfs/server/const.h"
#include "ssa/main/common/obj.h"
#include "common/errno.h"
#include <stdio.h>

namespace server {


int 
DirInode::Link(Session* session, const char* name, uint64_t ino)
{
	printf("LINK\n");
	if (Inode::type(ino) != kFileInode) {
		return -1;
	}

	// assign parent of underlying object
	// set link count

	return E_SUCCESS;
}


} // namespace server
