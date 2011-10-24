#include "mfs/client/sb.h"
#include <pthread.h>
#include "client/backend.h"
#include "mfs/psb.h"
#include "mfs/client/dir_inode.h"
#include "mfs/client/inode_factory.h"

namespace mfs {

// TODO:
// most of the functionality is quite generic so it should really be provided by the inode manager. 
// the superblock should only provide file-system functionality, that is construction of the 
// inode. it should behave as a factory. that is, re-route the request to the local factory. 
// Another approach would be for each file-system to define and register object factories
// with the generic inode manager (and storage manager). 
// The bottom line is that we need some form of polymorphism, either at the superblock or at the factory.

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
	client::Inode*              ip;
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

	pthread_mutex_lock(&mutex_);
	imap_->Insert(ip);
	ip->Get();
	pthread_mutex_unlock(&mutex_);
	*ipp = ip;
	return 0;
}


int
SuperBlock::GetInode(client::InodeNumber ino, client::Inode** ipp)
{
	client::Inode* ip;
	Pnode*         pnode;
	int            ret;

	pthread_mutex_lock(&mutex_);
	if ((ret = imap_->Lookup(ino, &ip))==0) {
		ip->Get();
		goto done;
	}
	
	assert((ret = InodeFactory::Open(this, ino, &ip))==0);

	ip->Get();
	imap_->Insert(ip);

done:
	pthread_mutex_unlock(&mutex_);
	*ipp = ip;
	return 0;
}


int
SuperBlock::PutInode(client::Inode* ip)
{
	pthread_mutex_lock(&mutex_);
	ip->Put();
	// TODO: if refcount == 0, do we remove inode from imap? 
	// Only if inode does not have any updates that need to be published.
	// Otherwise, we let the inode manager remove it when the inode is published.
	pthread_mutex_unlock(&mutex_);
}


client::Inode*
SuperBlock::WrapInode()
{

}

} // namespace mfs
