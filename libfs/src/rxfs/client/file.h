#ifndef __STAMNOS_RXFS_CLIENT_FILE_H
#define __STAMNOS_RXFS_CLIENT_FILE_H

#include <stdint.h>
#include <pthread.h>
#include <vector>
#include <boost/dynamic_bitset.hpp>
#include "rxfs/client/file_inode.h"

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
		  writable_(false),
		  append_(false)
	{ 
		pthread_mutex_init(&mutex_, NULL);
	}

	int Init(InodeNumber ino, int flags);

	int Write(Session* session, const char* src, uint64_t n);
	int Read(Session* session, char* dst, uint64_t n);
	int Write(Session* session, const char* src, uint64_t n, uint64_t offset);
	int Read(Session* session, char* dst, uint64_t n, uint64_t offset);
	uint64_t Seek(client::Session* session, uint64_t offset, int whence);
	int Release();

private:
	pthread_mutex_t mutex_;
	int             ref_;       // reference count
	uint64_t        off_;       // file offset
	FileInode       ip_;        // file inode
	bool            readable_;
	bool            writable_;
	bool            append_;
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

#endif // __STAMNOS_RXFS_CLIENT_FILE_H
