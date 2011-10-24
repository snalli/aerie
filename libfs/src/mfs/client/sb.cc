#include "mfs/client/sb.h"
#include "client/backend.h"
#include "mfs/pstruct.h"
#include "mfs/client/dinode.h"

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


int
SuperBlock::AllocInode(client::Session* session, int type, client::Inode** ipp)
{
	client::Inode*            ip;
	DirPnode<client::Session>*  dpnode;
	
	printf("mfs::SuperBlock::AllocInode\n");
	
	switch (type) {
		case client::type::kFileInode:
			//ip = new 	
			break;
		case client::type::kDirInode:
			dpnode = new(session) DirPnode<client::Session>;
			ip = new DirInodeMutable(this, dpnode);
			break;
	}

	*ipp = ip;
	return 0;
}


int
SuperBlock::GetInode(client::InodeNumber ino, client::Inode** ipp)
{
	client::Inode* ip;
	Pnode*         pnode;
	int            ret;

	if ((ret = imap_->Lookup(ino, &ip))==0) {
		//FIXME: bump the inode's ref count???
		*ipp = ip;
		return 0;
	}
	

	pnode = Pnode::Load(ino);
	//FIXME: persistent inode should have magic number identifying its type
	/*
	switch(pnode->magic_) {
		case 1: // directory
			minode = new mfs::DirInodeMutable(pnode);
			break;

		case 2: // file
			//FIXME
			//minode = new mfs::FileInodeMutable(pnode);
			break;
	}
	*/
	ip = new DirInodeMutable(this, pnode);

	//FIXME: what should be the inode's ref count??? 1?
	imap_->Insert(ip);
	*ipp = ip;

	return 0;
}


client::Inode*
SuperBlock::WrapInode()
{

}

} // namespace mfs
