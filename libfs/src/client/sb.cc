#ifndef _CLIENT_SUPERBLOCK_H_ANM198
#define _CLIENT_SUPERBLOCK_H_ANM198

#include "common/types.h"
#include "client/sb.h"
#include "client/inode.h"
#include "client/imap.h"

namespace client {

SuperBlock::SuperBlock(Session* session)
{
	pthread_mutex_init(&mutex_, NULL);
	imap_ = new InodeMap();
}


// this returns the allocated inode as write locked
int
SuperBlock::AllocInode(Session* session, int type, Inode** ipp)
{
	int    ret;
	Inode* ip;

	ret = MakeInode(session, type, &ip);
	
	pthread_mutex_lock(&mutex_);
	imap_->Insert(ip);
	ip->Get();
	pthread_mutex_unlock(&mutex_);

	*ipp = ip;

	return 0;
}


int
SuperBlock::GetInode(InodeNumber ino, client::Inode** ipp)
{
	client::Inode* ip;
	int            ret;

	pthread_mutex_lock(&mutex_);
	if ((ret = imap_->Lookup(ino, &ip))==0) {
		ip->Get();
		goto done;
	}
	
	assert((ret = LoadInode(ino, &ip)) == 0);

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

} // namespace client

#endif // _CLIENT_SUPERBLOCK_H_ANM198
