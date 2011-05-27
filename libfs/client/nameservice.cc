#include "nameservice.h"
#include <assert.h>
#include <fcntl.h>
#include <stdint.h>
#include <stdio.h>
#include <sys/mman.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <unistd.h>
#include <cstring>
#include "common/util.h"
#include "common/hrtime.h"




NameService::NameService(rpcc* c, unsigned int principal_id): 
	client_(c), 
	principal_id_(principal_id)
{
}


int
NameService::Lookup(const char *name, inode_t **inode)
{
	std::string        name_str;
	unsigned long long r;
	int                intret;

	name_str = std::string(name);

	intret = client_->call(25, principal_id_, name_str, r);
	if (intret) {
		return -intret;
	}
	*inode = (void*) r;
	
	return 0;
}


int
NameService::Link(const char *name, inode_t *inode)
{
	std::string        name_str;
	unsigned long long ino;
	int                r;
	int                intret;

	name_str = std::string(name);
	ino = (unsigned long long) inode;

	printf("NameService::Link DO\n");
	printf("NameService::Link client=%p\n", client_);
	intret = client_->call(26, principal_id_, name_str, ino, r);
	if (intret) {
		return -intret;
	}
	printf("NameService::Link DONE\n");

	return 0;
}


int
NameService::Remove(const char *name)
{
	std::string        name_str;
	int                r;
	int                intret;

	name_str = std::string(name);

	intret = client_->call(27, principal_id_, name_str, r);
	if (intret) {
		return -intret;
	}

	return 0;
}
