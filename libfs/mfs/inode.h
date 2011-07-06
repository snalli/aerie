#ifndef __INODE_H_AKE111
#define __INODE_H_AKE111

#include <pthread.h>
#include <stdint.h>
#include "common/interval_tree.h"
#include "mfs/pinode.h"

// FIXME: API needs a way to alloc new inode, 

class Inode 
{
public:
	Inode();
	Inode(PInode*);
	~Inode();

	int Read(char*, uint64_t, uint64_t);
	int Write(char*, uint64_t, uint64_t);
	
private:
	pthread_mutex_t* mutex_;
	PInode*          immpinode_;     // immutable pinode
	PInode*          mpinode_;       // mutable pinode
	IntervalTree*    intervaltree_;
	IntervalTree*    preconstructed_intervaltree_;

	int ReadImmutable(char*, uint64_t, uint64_t);
	int ReadMutable(char*, uint64_t, uint64_t);
	int WriteImmutable(char*, uint64_t, uint64_t);
	int WriteMutable(char*, uint64_t, uint64_t);
};

#endif /* __INODE_H_AKE111 */
