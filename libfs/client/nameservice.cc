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
#include "server/api.h"


NameService::NameService(rpcc* c, unsigned int principal_id, const char* namespace_name): 
	client_(c), 
	principal_id_(principal_id)
{
	strcpy(namespace_name_, namespace_name);
}


int
NameService::Lookup(const char *name, void **obj)
{
	std::string        name_str;
	unsigned long long r;
	int                intret;

	name_str = std::string(name);

	intret = client_->call(RPC_NAME_LOOKUP, principal_id_, name_str, r);
	if (intret) {
		return -intret;
	}
	*obj = (void*) r;
	
	return 0;
}


int
NameService::Link(const char *name, void *obj)
{
	std::string        name_str;
	unsigned long long val;
	int                r;
	int                intret;

	name_str = std::string(name);
	val = (unsigned long long) obj;

	intret = client_->call(RPC_NAME_LINK, principal_id_, name_str, val, r);
	if (intret) {
		return -intret;
	}

	return 0;
}


int
NameService::Unlink(const char *name)
{
	std::string        name_str;
	int                r;
	int                intret;

	name_str = std::string(name);

	intret = client_->call(RPC_NAME_UNLINK, principal_id_, name_str, r);
	if (intret) {
		return -intret;
	}

	return 0;
}
