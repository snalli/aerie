#ifndef _FILE_H_AFA191
#define _FILE_H_AFA191

#include <pthread.h>
#include <boost/dynamic_bitset.hpp>
#include "client/inode.h"

namespace client {

class File
public:

private:
	int       ref;       // reference count
	uint64_t  off;       // file offset
	Inode*    ip;        // file inode
	char      readable;
	char      writeable;
};


class FileManager {
public:
	FileManager(int fdmin, int fdmax)
		: fdmin_(fdmin),
		  fdmax_(fdmax-1)
	{}

	int Init();
	int Get();
	int Put(int fd);
	//File(int fd);
private:
	int Alloc(int start);

	pthread_mutex_t         mutex_;
	boost::dynamic_bitset<> fdset_;
	int                     fdmin_; // smaller file descriptor manageable by *this
	int                     fdmax_; // larger file descriptor manageable by *this

};




} // namespace client

#endif /* _FILE_H_AFA191 */
