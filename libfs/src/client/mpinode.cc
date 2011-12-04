#include "client/mpinode.h"
#include <string.h>
#include <stdio.h>
#include "common/util.h"
#include "common/errno.h"

namespace client {

int MPInode::Lookup(Session* session, const char* name, Inode** inodep)
{
	Inode* inode;
	int    len;
	int    i;

	if (name[0] == '\0') {
		return -E_INVAL;
	}

	// look up for mounted entries 
	for (i=0; i<entries_count_; i++) {
		if (strcmp(entries_[i].name_, name) == 0) {
			inode = entries_[i].inode_;
			inode->Get();
			*inodep = inode;
			return E_SUCCESS;
		}
	}

	// if no mount-point found then the caller should check the OS file-system 
	return -E_KVFS; 
}


// Assumes the caller has checked that the mounted file system does not 
// contain the name
int 
MPInode::Link(Session* session, const char* name, Inode* inode, bool overwrite)
{
	int    len;
	int    i;
	int    empty = entries_count_;
	Inode* ip;

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

} // namespace client
