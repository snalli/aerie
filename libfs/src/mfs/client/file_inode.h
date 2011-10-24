#ifndef __FILE_INODE_CLIENT_H_AKE111
#define __FILE_INODE_CLIENT_H_AKE111

#include <pthread.h>
#include <stdint.h>
#include "common/interval_tree.h"
#include "mfs/file_pnode.h"

// FIXME: API needs a way to alloc new inode, 

class FileInode 
{
public:
	FileInode();
	FileInode(FilePnode*);
	~FileInode();

	int Read(char*, uint64_t, uint64_t);
	int Write(char*, uint64_t, uint64_t);
	int Publish();
	
private:
	int ReadImmutable(char*, uint64_t, uint64_t);
	int ReadMutable(char*, uint64_t, uint64_t);
	int WriteImmutable(char*, uint64_t, uint64_t);
	int WriteMutable(char*, uint64_t, uint64_t);

	pthread_mutex_t*   mutex_;
	FilePnode*         pinode_;        // pinode
	bool               pinodeism_;     // pinode is mutable
	FilePnode::Region* region_;        // mutable region
	IntervalTree*      intervaltree_;
	uint64_t           size_;
};

#endif /* __FILE_INODE_H_AKE111 */
