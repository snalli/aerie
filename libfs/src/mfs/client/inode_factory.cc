#include "mfs/client/inode_factory.h"
#include "client/backend.h"
#include "mfs/client/dir_inode.h"

namespace mfs {

int InodeFactory::Open(client::SuperBlock* sb, client::InodeNumber ino, 
                       client::Inode** ipp)
{
	Pnode* pnode = Pnode::Load(ino);
	switch(pnode->magic_) {
		//case 1: // directory
		default:
			*ipp = new DirInodeMutable(sb, pnode);
			break;
		case 2: // file
			//FIXME
			//*ipp = new mfs::FileInodeMutable(sb, pnode);
			break;
	}
	return 0;
}


} // namespace mfs
