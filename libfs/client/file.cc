#include "client/fdmgr.h"
#include <iostream>

namespace client {

int 
FileManager::Init()
{
	fdset_.resize(fdmax_ - fdmin_ + 1);
	pthread_mutex_init(&mutex_, NULL);
}


static boost::dynamic_bitset<>::size_type
find_first_zero(boost::dynamic_bitset<>& fdset, 
                boost::dynamic_bitset<>::size_type start)
{
	boost::dynamic_bitset<>::size_type first_bit_set;
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

	pthread_mutex_lock(&mutex_);
	first_bit_zero = find_first_zero(fdset_, start);

	if (first_bit_zero == boost::dynamic_bitset<>::npos) {
		pthread_mutex_unlock(&mutex_);
		return -1;
	}
	
	fd = first_bit_zero;
	assert(fdset_[fd] == 0);
	fdset_[fd] = 1;

	pthread_mutex_unlock(&mutex_);
	return fdmin_ + fd;
}

int 
FileManager::Get()
{
	return AllocFd(0);
}


int 
FileManager::Put(int fd)
{
	if (fd > fdmax_ || fd < fdmin_) {
		return -1;
	}
	pthread_mutex_lock(&mutex_);
	fdset_[fd-fdmin_] = 0;
	pthread_mutex_unlock(&mutex_);
	return 0;
}


// returns a file descriptor
int 
FileManager::AllocFile(File** fpp)
{
	

}


// returns a file descriptor
int
FileManager::Duplicate(int fd)
{


}


} // namespace client

#if 0

static int
fd2file(int fd, struct file **pf)
{
  struct file *f;

  if(fd < 0 || fd >= NOFILE || (f=proc->ofile[fd]) == 0)
    return -1;
  if(pf)
    *pf = f;
  return 0;
}


// Allocate a file descriptor for the given file.
// Takes over file reference from caller on success.
fdalloc(struct file *f)
{
  int fd;

  for(fd = 0; fd < NOFILE; fd++){
    if(proc->ofile[fd] == 0){
      proc->ofile[fd] = f;
      return fd;
    }
  }
  return -1;
}

int
libfs_init()
{
	binit();
	iinit();
	fileinit();
	scminit();
  	proc->cwd = namei("/");
	return 0;
}

int
libfs_dup(int fd)
{
  struct file *f;
  
  if (fd2file(fd, &f) < 0)
    return -1;
  if((fd=fdalloc(f)) < 0)
    return -1;
  filedup(f);
  return fd;
}

#endif
