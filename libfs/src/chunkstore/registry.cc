#include "registry.h"
#include <assert.h>
#include <fcntl.h>
#include <stdint.h>
#include <stdio.h>
#include <sys/mman.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <unistd.h>
#include <cstring>
#include "server/api.h"
#include "common/util.h"
#include "common/hrtime.h"


Registry::Registry(rpcc* c, unsigned int principal_id): 
	client_(c), 
	principal_id_(principal_id)
{
}


int
Registry::Lookup(const char *name, uint64_t* val)
{
	std::string        name_str;
	unsigned long long r;
	int                intret;

	name_str = std::string(name);

	intret = client_->call(RPC_REGISTRY_LOOKUP, principal_id_, name_str, r);
	if (intret) {
		return -intret;
	}
	*val = r;
	
	return 0;
}


int
Registry::Add(const char *name, uint64_t val)
{
	std::string        name_str;
	int                r;
	int                intret;

	name_str = std::string(name);

	intret = client_->call(RPC_REGISTRY_ADD, principal_id_, name_str, (unsigned long long) val, r);
	if (intret) {
		return -intret;
	}

	return 0;
}


int
Registry::Remove(const char *name)
{
	std::string        name_str;
	int                r;
	int                intret;

	name_str = std::string(name);

	intret = client_->call(RPC_REGISTRY_REMOVE, principal_id_, name_str, r);
	if (intret) {
		return -intret;
	}

	return 0;
}
