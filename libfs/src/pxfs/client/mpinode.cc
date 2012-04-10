#include "pxfs/client/mpinode.h"
#include <string.h>
#include <stdio.h>
#include "common/util.h"
#include "common/errno.h"
#include "common/hrtime.h"

namespace client {

int MPInode::Lookup(Session* session, const char* name, int flags, Inode** inodep)
{
	HRTIME_DEFINITIONS
	Inode* inode;
	int    i;

	if (name[0] == '\0') {
		return -E_INVAL;
	}

	HRTIME_SAMPLE
	// look up for mounted entries 
	for (i=0; i<entries_count_; i++) {
		if (strcmp(entries_[i].name_, name) == 0) {
			inode = entries_[i].inode_;
			inode->Get();
			*inodep = inode;
			HRTIME_SAMPLE
			return E_SUCCESS;
		}
	}
	HRTIME_SAMPLE

	// if no mount-point found then the caller should check the OS file-system 
	return -E_KVFS; 
}


// Assumes the caller has checked that the mounted file system does not 
// contain the name
int 
MPInode::Link(Session* session, const char* name, Inode* inode, bool overwrite)
{
	int    i;
	int    empty = entries_count_;

	for (i=0; i<entries_count_; i++) {
		if (strcmp(entries_[i].name_, name) == 0) {
			return -1;
		}
		if (*entries_[i].name_ == '\0') {
			empty = i;
		}
	}
	if (empty<MAX_NUM_ENTRIES) {
		entries_count_++;
		strcpy(entries_[empty].name_, name);
		entries_[empty].inode_ = inode;
	}	

	return 0;
}

int 
MPInode::Lock(::client::Session* session, Inode* parent_inode, lock_protocol::Mode mode)
{
	return E_SUCCESS;
}


int 
MPInode::Lock(::client::Session* session, lock_protocol::Mode mode)
{
	return E_SUCCESS;
}


int 
MPInode::Unlock(::client::Session* session)
{
	return E_SUCCESS;
}
	

} // namespace client
