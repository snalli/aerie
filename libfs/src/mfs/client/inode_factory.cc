#include "mfs/client/inode_factory.h"
#include "client/backend.h"
#include "mfs/client/dir_inode.h"

namespace mfs {
namespace client {


int
InodeFactory::Make()
{
	//dpo::containers::client::SuperContainer::Object::Make()

}


int 
InodeFactory::Load()
{


}


int InodeFactory::Open(::client::SuperBlock* sb, InodeNumber ino, 
                       ::client::Inode** ipp)
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


} // namespace client 
} // namespace mfs
