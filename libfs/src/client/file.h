#ifndef _FILE_H_AFA191
#define _FILE_H_AFA191

#include <stdint.h>
#include <pthread.h>
#include <vector>
#include <boost/dynamic_bitset.hpp>
#include "client/inode.h"

namespace client {

class Session;
class FileManager;

class File {
friend class FileManager;
public:
	File()
		: ref_(0),
		  off_(0),
		  ip_(0),
		  readable_(false),
		  writable_(false)
	{ 
		pthread_mutex_init(&mutex_, NULL);
	}

	int Write(Session* session, const char* src, uint64_t n);
	int Read(Session* session, char* dst, uint64_t n);
	int Release();

private:
	pthread_mutex_t mutex_;
	int             ref_;       // reference count
	uint64_t        off_;       // file offset
	Inode*          ip_;        // file inode
	bool            readable_;
	bool            writable_;
};


class FileManager {
public:
	FileManager(int fdmin, int fdmax)
		: fdmin_(fdmin),
		  fdmax_(fdmax-1)
	{}

	int Init();
	int AllocFd(File* fp);
	int AllocFile(File** fpp);
	int ReleaseFile(File* fp);
	int Get(int fd, File** fpp);
	int Put(int fd);
	int	Lookup(int fd, File** fpp);

private:
	int AllocFd(int start);

	pthread_mutex_t         mutex_;
	boost::dynamic_bitset<> fdset_;
	std::vector<File*>      ftable_;
	int                     fdmin_; // smaller file descriptor manageable by *this
	int                     fdmax_; // larger file descriptor manageable by *this

};




} // namespace client

#endif /* _FILE_H_AFA191 */
