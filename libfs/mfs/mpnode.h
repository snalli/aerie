#ifndef _MPNODE_H_JAK129
#define _MPNODE_H_JAK129

#include <stdint.h>
#include "mfs/inode.h"

// A pseudo inode representing a mount point


const int MAX_NUM_ENTRIES=2;



struct Entry {
	char*  name_;
	Inode* inode_;
};


class MPInode: public Inode {
public:
	
	int Lookup(char* name);
	int Insert(char* name, Inode* inode);

private:
	
	Inode* parent_;
	Entry  entries_[MAX_NUM_ENTRIES];
	int    entries_count_;
	//FileSystem* mounted_fs_;
};


int MPInode::Lookup(char* name)
{
	int len;

	// handle special cases '.' and '..'
	if (name[0] == '\0') {
		return -1;
	} else if (name[1] == '\0') {
		if (name[0] == '.' && name[1] == '\0') {
			return this;
		} 
		len = 1;
	} else if (name[2] == '\0') {
		if (name[0] == '.' && name[1] == '.') {
			return parent_;
		} 
		len = 2;
	}

	for (i=0; i<entries_count_; i++) {
		if (strcmp(entries_[i].name_, name) == 0) {
			return entries_[i].inode_;
		}
	}

	return mounted_fs_->RootInode();
}

int MPInode::Insert(char* name, Inode* inode)
{
	int len;

	
}


#endif /* _MPNODE_H_JAK129 */
