#ifndef _CLIENT_SUPERBLOCK_H_ANM198
#define _CLIENT_SUPERBLOCK_H_ANM198

#include "common/types.h"
#include "client/client_i.h"
#include "client/sb.h"
#include "client/inode.h"

namespace client {

#if 0

SuperBlock::SuperBlock(Session* session)
{
	pthread_mutex_init(&mutex_, NULL);
	imap_ = new InodeMap();
	if (global_hlckmgr) {
		global_hlckmgr->RegisterLockUser(this);
	}
}


// this returns the allocated inode as write locked
int
SuperBlock::AllocInode(Session* session, int type, Inode* parent, Inode** ipp)
{
	int    ret;
	Inode* ip;

	ret = MakeInode(session, type, &ip);
	
	pthread_mutex_lock(&mutex_);
	imap_->Insert(ip);
	ip->Lock(parent, lock_protocol::Mode::XR);
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

	dbg_log (DBG_INFO, "Get pnode %llu\n", ino);

	pthread_mutex_lock(&mutex_);
	if ((ret = imap_->Lookup(ino, &ip)) == 0) {
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


void
SuperBlock::OnRelease(HLock* hlock)
{
	client::Inode* ip;
	InodeNumber    ino;
	int            ret;

	dbg_log (DBG_INFO, "CALLBACK: Releasing hierarchical lock %s\n", hlock->lid_.c_str());
	
	pthread_mutex_lock(&mutex_);
	ino = hlock->lid_.number();
	google::dense_hash_map<InodeNumber, Inode*>::iterator itr;
	for (itr=imap_->ino2inode_map_.begin(); itr != imap_->ino2inode_map_.end(); itr++) {
		printf("%llu\n", itr->first);
	}
	if ((ret = imap_->Lookup(ino, &ip)) == 0) {
		ip->Publish(global_session);
	}
	pthread_mutex_unlock(&mutex_);

}


void
SuperBlock::OnConvert(HLock* hlock)
{
	dbg_log (DBG_INFO, "CALLBACK: Converting hierarchical lock %s\n", hlock->lid_.c_str());
	
	// do nothing?
}

#endif


} // namespace client

#endif // _CLIENT_SUPERBLOCK_H_ANM198
