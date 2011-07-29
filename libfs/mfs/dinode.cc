#include "mfs/dinode.h"
#include <stdint.h>
#include "mfs/inode.h"





int 
DirInodeImmutable::Lookup(char* name)
{
	uint64_t val;

	return snode_->ht_->Search(name, strlen(name)+1, &val);

}
