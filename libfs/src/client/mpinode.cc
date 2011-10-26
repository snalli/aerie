#include "client/mpinode.h"
#include <string.h>
#include <stdio.h>
#include "common/util.h"
#include "common/errno.h"

using namespace client;

int MPInode::Lookup(Session* session, const char* name, Inode** inode)
{
	int len;
	int i;

	if (name[0] == '\0') {
		return -E_INVAL;
	}

	// handle special cases '.' and '..'
	switch (str_is_dot(name)) {
		case 1: // '.'
			*inode = this;
			return E_SUCCESS;
		case 2: // '..'
			*inode = parent_;
			return E_SUCCESS;
	}

	// look up for mounted entries 
	for (i=0; i<entries_count_; i++) {
		if (strcmp(entries_[i].name_, name) == 0) {
			*inode = entries_[i].inode_;
			return E_SUCCESS;
		}
	}

	return -E_NOENT;
}

// Assumes the caller has checked that the mounted file system does not 
// contain the name
int MPInode::Insert(Session* session, const char* name, Inode* inode)
{
	int    len;
	int    i;
	int    empty;
	Inode* ip;

	for (i=0, empty=0; i<entries_count_; i++) {
		if (strcmp(entries_[i].name_, name) == 0) {
			return -1;
		}
		empty = i;
	}
	if (empty<MAX_NUM_ENTRIES) {
		entries_count_++;
		strcpy(entries_[empty].name_, name);
		entries_[empty].inode_ = inode;
	}	

	return 0;
}
