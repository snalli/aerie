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
	if (flags & O_RDWR) {
		readable_ = true;
		writable_ = true;
	} else if (flags & O_RDONLY) {
		readable_ = true;
	} else if (flags & O_WRONLY) {
		writable_ = true;
	}
	ip_ = ip;
	return E_SUCCESS;
}



int 
File::Write(client::Session* session, const char* src, uint64_t n)
{
	int ret;

	if (writable_ == false) {
		return -1;
	}

	pthread_mutex_lock(&mutex_);
	session->journal()->TransactionBegin();
	session->journal() << Publisher::Message::LogicalOperation::Write(ip_->ino());
	if ((ret = ip_->Write(session, const_cast<char*>(src), off_, n)) > 0) {
		off_ += ret;
	}
	session->journal()->TransactionCommit();
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

	if ((ret=ip_->Read(session, dst, off_, n)) > 0) {
		off_+=ret;
	}

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
	session->journal()->TransactionBegin();
	session->journal() << Publisher::Message::LogicalOperation::Write(ip_->ino());
	ret = ip_->Write(session, const_cast<char*>(src), offset, n);
	session->journal()->TransactionCommit();
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
	ret=ip_->Read(session, dst, offset, n);
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
	//FIXME: release inode (put)
	return E_SUCCESS;
} 



// FileManager implementation

int 
FileManager::Init()
{
	fdset_.resize(fdmax_ - fdmin_ + 1);
	ftable_.resize(fdmax_ - fdmin_ + 1);
	pthread_mutex_init(&mutex_, NULL);
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

	pthread_mutex_unlock(&mutex_);
	if ((fd = AllocFd(0)) < 0) {
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
	*fpp = new File();
	(*fpp)->ref_ = 1;
	return 0;
}


int 
FileManager::ReleaseFile(File* fp)
{
	fp->Release();
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

	pthread_mutex_unlock(&mutex_);
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

	pthread_mutex_unlock(&mutex_);
	fp = ftable_[fd-fdmin_];
	if ((newfd = AllocFd(0)) < 0) {
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
	return ReleaseFile(fp);
}


} // namespace client
