#define  __CACHE_GUARD__

#include "pxfs/client/file.h"
#include <iostream>
#include "pxfs/client/client_i.h"
#include "pxfs/client/session.h"
#include "pxfs/common/publisher.h"
#include "common/errno.h"

// FIXME BUG Data race: 
//       Lookup may race with a concurrent ReleaseFile and return a 
//       File object that is concurrently released and is no longer
//       valid. This is a problem with any file operation concurrent 
//       with a file close.


namespace client {

// File implementation

int 
File::Init(Inode* ip, int flags)
{
	
	//printf("\n Sanketh : Inside File::Init...\n");
	if (flags & O_APPEND) {
		append_ = true;
	}
	readable_ = true; // O_RDONLY == 0, so file is readonly unless someone passes O_WRONLY
	if (flags & O_RDWR) {
		readable_ = true;
		writable_ = true;
	} else if (flags & O_WRONLY) {
		readable_ = false;
		writable_ = true;
	}
	ip_ = ip;
	return E_SUCCESS;
}



int 
File::Write(client::Session* session, const char* src, uint64_t n)
{
	////printf("\n Sanketh : Inside File::Write...\n");
	int      ret;
	uint64_t size;

	if (writable_ == false) {
		return -1;
	}
	s_log("[%ld] File::%s",s_tid, __func__);
	pthread_mutex_lock(&mutex_);
	ip_->Lock(session, lock_protocol::Mode::XL);
	// insight : You should have removed the compatibility check for locks
	// from the lock manager !
	session->journal()->TransactionBegin();
	session->journal() << Publisher::Message::LogicalOperation::Write(ip_->ino());
	if (append_) {
		ip_->ioctl(session, Inode::kSize, (void*) &size);
		off_ = size;
	}
	if ((ret = ip_->Write(session, const_cast<char*>(src), off_, n)) > 0) {
		off_ += ret;
		// insight : off_ is simply a variable to keep track of where our last write ended !
	}
	session->journal()->TransactionCommit();
	ip_->Unlock(session);
	pthread_mutex_unlock(&mutex_);

	return ret;
}


int
File::Read(client::Session* session, char* dst, uint64_t n)
{
	int ret;

	if (readable_ == false) {
		return -1;
	}

	pthread_mutex_lock(&mutex_);
	ip_->Lock(session, lock_protocol::Mode::SL);
	if ((ret=ip_->Read(session, dst, off_, n)) > 0) {
		off_+=ret;
	}
	ip_->Unlock(session);
	pthread_mutex_unlock(&mutex_);

	return ret;
}


int 
File::Write(client::Session* session, const char* src, uint64_t n, uint64_t offset)
{
	int ret;

	if (writable_ == false) {
		return -1;
	}

	pthread_mutex_lock(&mutex_);
	ip_->Lock(session, lock_protocol::Mode::XL);
	session->journal()->TransactionBegin();
	session->journal() << Publisher::Message::LogicalOperation::Write(ip_->ino());
	ret = ip_->Write(session, const_cast<char*>(src), offset, n);
	session->journal()->TransactionCommit();
	ip_->Unlock(session);
	pthread_mutex_unlock(&mutex_);

	return ret;
}


int
File::Read(client::Session* session, char* dst, uint64_t n, uint64_t offset)
{
	int ret;

	if (readable_ == false) {
		return -1;
	}

	pthread_mutex_lock(&mutex_);
	ip_->Lock(session, lock_protocol::Mode::SL);
	ret=ip_->Read(session, dst, offset, n);
	ip_->Unlock(session);
	pthread_mutex_unlock(&mutex_);

	return ret;
}



uint64_t
File::Seek(client::Session* session, uint64_t offset, int whence)
{
	uint64_t size;
	switch(whence) {
		case SEEK_SET:
			off_ = offset;
			break;
		case SEEK_CUR:
			off_ += offset;
			break;
		case SEEK_END:
			ip_->ioctl(session, Inode::kSize, (void*) &size);
			off_ = size + offset;
			break;
	}
	return off_;
}


// called when the File object is no longer referenced by any file descriptor
int 
File::Release() 
{
        s_log("[%ld] File::%s",s_tid, __func__);

	ip_->Put();
        //s_log("[%ld] (R) File::%s",s_tid, __func__);
	return E_SUCCESS;
} 



// FileManager implementation

int 
FileManager::Init()
{
//	printf("\nInitializing File Manager...");
	fdset_.resize(fdmax_ - fdmin_ + 1);
	ftable_.resize(fdmax_ - fdmin_ + 1);
	pthread_mutex_init(&mutex_, NULL);
//	printf("\nSUCCESS");
	return E_SUCCESS;
}


static boost::dynamic_bitset<>::size_type
find_first_zero(boost::dynamic_bitset<>& fdset, 
                boost::dynamic_bitset<>::size_type start)
{
	boost::dynamic_bitset<>::size_type first_bit_zero;
	boost::dynamic_bitset<>::size_type bit_set;
	boost::dynamic_bitset<>::size_type prev_bit_set;

	prev_bit_set = start-1; // If start is 0 then 
	                        // start-1 == -1 == boost::dynamic_bitset<>::npos
	for (bit_set = fdset.find_first();
	     bit_set != boost::dynamic_bitset<>::npos;
	     prev_bit_set = bit_set, bit_set = fdset.find_next(bit_set))
	{
		if (bit_set - (prev_bit_set+1) > 0) {
			break;
		}
	}
	first_bit_zero = ++prev_bit_set;
	if (first_bit_zero >= fdset.size()) {
		first_bit_zero = boost::dynamic_bitset<>::npos;
	}

	return first_bit_zero;
}


int
FileManager::AllocFd(int start)
{
	int                                fd;
	boost::dynamic_bitset<>::size_type first_bit_zero;

//	printf("\n @ Allocating fd");
	first_bit_zero = find_first_zero(fdset_, start);
	if (first_bit_zero == boost::dynamic_bitset<>::npos) {
		return -1;
	}
	
	fd = first_bit_zero;
	assert(fdset_[fd] == 0);
	fdset_[fd] = 1;

	return fdmin_ + fd;
}


// Allocate a file descriptor and assign File fp to it.
int
FileManager::AllocFd(File* fp)
{
	int fd;

//	printf("\n @ Allocating fd");
	pthread_mutex_lock(&mutex_);
	if ((fd = AllocFd(0)) < 0) { // insight : I don't understand this.
		pthread_mutex_unlock(&mutex_);
		return -1;
	}
	assert(ftable_[fd-fdmin_] ==0);
	ftable_[fd-fdmin_] = fp;
	pthread_mutex_unlock(&mutex_);
	return fd;
}


int 
FileManager::AllocFile(File** fpp)
{
//	printf("\n @ Allocating fp");
	*fpp = new File();
	(*fpp)->ref_ = 1;
	return 0;
}


int 
FileManager::ReleaseFile(File* fp)
{
        s_log("[%ld] FileManager::%s",s_tid, __func__);

	fp->Release();
        //s_log("[%ld] (R) FileManager::%s",s_tid, __func__);
	delete fp;
	return 0;
}


int
FileManager::Lookup(int fd, File** fpp)
{
	File* fp;

	if (fd > fdmax_ || fd < fdmin_) {
		return -E_KVFS;
	}

	pthread_mutex_lock(&mutex_);
	fp = ftable_[fd-fdmin_];
	pthread_mutex_unlock(&mutex_);

	*fpp = fp;
	return fd;
}


// Returns a new file descriptor referencing the File reference by fd.
// Bumps up the reference count.
int
FileManager::Get(int fd, File** fpp)
{
	File* fp;
	int   newfd;

	if (fd > fdmax_ || fd < fdmin_) {
		return -E_KVFS;
	}

	pthread_mutex_lock(&mutex_);
	fp = ftable_[fd-fdmin_];
	if ((newfd = AllocFd((int) 0)) < 0) {
		pthread_mutex_unlock(&mutex_);
		return -1;
	}
	assert(ftable_[newfd-fdmin_] == 0);
	ftable_[newfd-fdmin_] = fp;
	fp->ref_++;
	pthread_mutex_unlock(&mutex_);

	*fpp = fp;
	return newfd;
}


int
FileManager::Put(int fd)
{
	File* fp;

        s_log("[%ld] FileManager::%s %d",s_tid, __func__, fd);

	if (fd > fdmax_ || fd < fdmin_) {
		return -E_KVFS;
	}

	pthread_mutex_lock(&mutex_);

	fp = ftable_[fd-fdmin_];
	if (fp==0) {
		pthread_mutex_unlock(&mutex_);
		return -1;
	}
	fdset_[fd-fdmin_] = 0;
	ftable_[fd-fdmin_] = 0;
	if (--fp->ref_ > 0) {
		pthread_mutex_unlock(&mutex_);
		return 0;
	}
	pthread_mutex_unlock(&mutex_);
	// ref is 0, release the File object
	// we no longer need to hold the manager mutex as the file object
	// is unreachable.
	int ret = ReleaseFile(fp);
        //s_log("[%ld] (R) FileManager::%s %d",s_tid, __func__, fd);
	return ret;
}


} // namespace client
