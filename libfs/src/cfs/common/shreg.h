#ifndef __STAMNOS_COMMON_SHARED_REGION_H
#define __STAMNOS_COMMON_SHARED_REGION_H

#include <sys/mman.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <inttypes.h>
#include <sstream>
#include "bcs/main/common/debug.h"
#include "common/errno.h"


class SharedRegion {
public:
	SharedRegion(int clt) 	
		: clt_(clt)
	{ }
	
	int Create();
	int Open();
	int Init(int oflag);

	void* base() { return base_; }

private:
	int         clt_;
	std::string path_;
	uint64_t    size_;
	void*       base_;
};


inline int 
SharedRegion::Init(int oflag)
{
	int               ret;
	int               fd;
	const char*       root = "/shreg_";
	std::stringstream ss;

	ss << root;
	ss << clt_;

	path_ = ss.str();
	size_ = 1024*1024;

	dbg_log(DBG_DEBUG, "SharedRegion: path = %s, size = %lu\n", 
	        path_.c_str(), size_);
	
	if ((fd = shm_open(path_.c_str(), oflag | O_RDWR, S_IRWXU | S_IRWXG | S_IRWXO)) < 0) {
		return -E_NOMEM;
	}
	if (oflag & O_CREAT) {
		ftruncate(fd, size_);
	}
	if ((base_ = mmap(0, size_, PROT_READ|PROT_WRITE, MAP_SHARED, fd, 0)) == NULL) {
		return -E_NOMEM;
	}
	close(fd);
	return E_SUCCESS;
}


inline int 
SharedRegion::Create()
{
	return Init(O_CREAT|O_TRUNC);
}


inline int 
SharedRegion::Open()
{
	return Init(0);
}


#endif // __STAMNOS_COMMON_SHARED_REGION_H
