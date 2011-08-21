#include "mfs/sb.h"

namespace mfs {

client::Inode* 
SuperBlock::CreateImmutableInode(int t)
{
	client::Inode* inode;

	switch(t) {
		default:
			inode = new DirInodeImmutable(NULL);
			;
	}

	return inode;
}


client::Inode*
SuperBlock::CreateInode(int t)
{

}


client::Inode*
SuperBlock::WrapInode()
{

}

} // namespace mfs
